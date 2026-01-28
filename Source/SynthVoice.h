#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"

using namespace juce;

//===============================================================================
struct ChainSettings
{
    float Attack{ 0.0f }, Decay{ 0.0f }, Sustain{ 0.0f }, Release{ 0.0f };
    float PluckPos{ 0.0f }, BridgeRefCoeff{ 0.0f };
};

//===============================================================================
class Physical_Model_StringAudioProcessor;

class SynthVoice : public SynthesiserVoice
{
public:
    SynthVoice(Physical_Model_StringAudioProcessor* pSynth);
    ~SynthVoice() override;

    bool canPlaySound(SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void setADSR(float attack, float decay, float sustain, float release);
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void releaseResources();

    std::vector<float> createPluckShape(float pluck, int L);

    bool isMakingSound() const noexcept { return MainADSR.isActive(); }

private:
    Physical_Model_StringAudioProcessor* synth = nullptr;

    juce::ADSR MainADSR;
    juce::ADSR::Parameters MainADSRParams;

    ChainSettings chainsettings;

    double SampleRate = 44100.0;
    float level = 0.0f;
    float frequency = 440.0f;
    float r = 0.94f;          
    float Out = 0.0f;

    int L = 0;               
    int pickup = 0;           
    int pluck = 0;      

    std::vector<float> Left, Right, x;

    bool ismakingsound;

    stk::BiQuad filter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthVoice)
};


