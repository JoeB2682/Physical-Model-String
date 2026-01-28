/*
  ==============================================================================

    SynthVoice.cpp
    Created: 19 Oct 2025 3:37:24pm
    Author:  josep

  ==============================================================================
*/

#include "../Source/SynthVoice.h"
#include "PluginProcessor.h"

//===============================================================================
SynthVoice::SynthVoice(Physical_Model_StringAudioProcessor* pSynth) :
    synth(nullptr),
    ismakingsound(false),
    Out(0.0f)
{
    synth = pSynth;
    frequency = level = 0;

    Right.resize(L);
    Left.resize(L);
}

SynthVoice::~SynthVoice() {}
//===============================================================================
void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
    SampleRate = sampleRate;
    MainADSR.setSampleRate(sampleRate);
}

//===============================================================================
bool SynthVoice::canPlaySound(SynthesiserSound* sound) { return dynamic_cast<SynthSound*>(sound) != nullptr; }
//===============================================================================
void SynthVoice::setADSR(float Attack, float Decay, float Sustain, float Release)
{
    //Has to have slight initial value to prevent envelope object ramp error
    MainADSRParams.attack = Attack + 0.001f;
    MainADSRParams.decay = Decay;
    MainADSRParams.sustain = Sustain;
    MainADSRParams.release = Release;

    MainADSR.setParameters(MainADSRParams);
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition)
{
    synth->getChainSettings(chainsettings);

    setADSR(chainsettings.Attack, chainsettings.Decay, chainsettings.Sustain, chainsettings.Release);
    r = chainsettings.BridgeRefCoeff;

    level = velocity;
    frequency = MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    MainADSR.noteOn();

    if (frequency <= 0.0f || SampleRate <= 0.0)
    {
        L = 1;
    }
    else
    {
        //samples per period
        L = static_cast<int>(std::floor(SampleRate / frequency));
        if (L < 2) L = 2;
    }

    // pluck position (0 .. L-1)
    if (chainsettings.PluckPos == 0) {
        pluck = 0.1f;
    }
    else {
        pluck = chainsettings.PluckPos * (L - 1);

    }

    pickup = static_cast<int>(std::floor(L / 2.0f));

    // create excitation
    x = createPluckShape(pluck, L);

    // resize delay lines AFTER L is known
    Left.assign(L, 0.0f);
    Right.assign(L, 0.0f);

    for (int i = 0; i < L; ++i)
    {
        Left[i] = x[i] * 0.5f;
        Right[i] = x[i] * 0.5f;
    }
}


void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    MainADSR.noteOff(); 

    if (!allowTailOff || (!MainADSR.isActive()))
    {
        clearCurrentNote();
        MainADSR.reset();
    }
}
//===============================================================================
void SynthVoice::pitchWheelMoved(int newPitchWheelValue) {}
//===============================================================================
void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue) {}
//===============================================================================
std::vector<float> SynthVoice::createPluckShape(float pluck, int L) 
{
    int pluckIndex = std::floor(pluck);
    std::vector<float> x;
    x.reserve(L);

    // First segment: rising ramp (0 to 1)
    for (int i = 0; i <= pluckIndex; ++i)
        x.push_back(static_cast<float>(i) / pluck);

    // Second segment: falling ramp (1 to 0)
    for (int i = pluckIndex + 1; i < L; ++i)
        x.push_back((L - 1 - i) / (L - 1 - pluck));

    return x;
}

void SynthVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    if (L < 2 || Left.empty() || Right.empty())
        return;

    //========= Main Waveguide Loop =========
    for (int n = 0; n < numSamples; n++) {

        // Shift left-going wave safely
        if (L > 1 && Left.size() >= static_cast<size_t>(L))
        {
            for (int i = 0; i < L - 1; ++i)
                Left[i] = Left[i + 1];

            Left[L - 1] = 0.0f; // append dummy value
        }

        //At the 'nut' (left - hand end), assume perfect reflection(*-1).
        //New right - going value is negative of new value at nut of left - going
        float nut = -Left[0];

        //Add reflection from nut into first element of right - going delay line;
        //Shift right - going wave one step
        for (int i = L - 1; i > 0; i--)
            Right[i] = Right[i - 1];

        Right[0] = nut;  // prepend nut value

        //At the 'bridge' (right - hand end), assume perfect reflection(*-1).
        //New left - going value is negative of new value at bridge of right - going
        float bridge = -r * Right[L - 1];

        //Add new bridge value to end of left - going delay line, replacing dummy
        //value from above :
        Left[L - 1] = bridge;

        //Moving Average Filter(3 Point)
        /*
        for (int i = 1; i < L - 1; ++i)
            Left[i] = (Left[i - 1] + Left[i] + Left[i + 1]) / 3.0f;
        */

        //Biquad Filter 
        filter.setLowPass(15000, 0.71);

        for (int i = 1; i < L - 1; ++i)
            Left[i] = filter.tick(Left[i]);
            
        //Output is sum of left and right going delay lines at pickup point.
        // Calculate output :
        float OutSample = Left[pickup] + Right[pickup];

        float MainADSRenv = MainADSR.getNextSample(); 
        float finalSample = OutSample * MainADSRenv * level; 

        // write into all channels
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.addSample(channel, startSample + n, finalSample);
    }
    ++startSample;

    if (!MainADSR.isActive())
        clearCurrentNote();
}
//===============================================================================
void SynthVoice::releaseResources()
{
    MainADSR.reset();
}