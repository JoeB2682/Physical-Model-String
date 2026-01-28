/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Physical_Model_StringAudioProcessor::Physical_Model_StringAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), myVoice(nullptr), lastSampleRate(getSampleRate()),
                          forwardFFT(fftOrder),
                          window(fftSize, juce::dsp::WindowingFunction<float>::hamming)
#endif
{
    mySynth.clearVoices();

    for (int i = 0; i < 12; i++) {
        mySynth.addVoice(new SynthVoice(this));
    }

    mySynth.clearSounds();
    mySynth.addSound(new SynthSound());
}

Physical_Model_StringAudioProcessor::~Physical_Model_StringAudioProcessor()
{
}

//==============================================================================
const juce::String Physical_Model_StringAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Physical_Model_StringAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Physical_Model_StringAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Physical_Model_StringAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Physical_Model_StringAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Physical_Model_StringAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Physical_Model_StringAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Physical_Model_StringAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Physical_Model_StringAudioProcessor::getProgramName (int index)
{
    return {};
}

void Physical_Model_StringAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Physical_Model_StringAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ignoreUnused(samplesPerBlock); //Ignores Samples from last key pressed
    lastSampleRate = sampleRate;
    mySynth.setCurrentPlaybackSampleRate(lastSampleRate);

    for (int i = 0; i < mySynth.getNumVoices(); i++)
    {
        if (auto* voice = static_cast<SynthVoice*>(mySynth.getVoice(i)))
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }
}

void Physical_Model_StringAudioProcessor::releaseResources()
{
    // Release resources used by each voice
    for (int i = 0; i < mySynth.getNumVoices(); i++)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i)))
            voice->releaseResources();
    }

    // Clear FIFO and FFT data buffers
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    fifoIndex = 0;

    std::fill(fftData.begin(), fftData.end(), 0.0f);
    nextFFTBlockReady = false;

    numSamples = 0;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Physical_Model_StringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Physical_Model_StringAudioProcessor::pushNextSampleIntoFifo(float sample)
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[(size_t)fifoIndex++] = sample;
}

void Physical_Model_StringAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    getChainSettings(processorChainsettings);

    numSamples = buffer.getNumSamples();

    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);

    mySynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    auto* channelData = buffer.getReadPointer(0);

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        float sample = channelData[i];
        float fullrangesample = juce::jlimit(-1.0f, 1.0f, sample);

        int index = waveformWriteIndex.fetch_add(1);
        waveformBuffer[(size_t)(index % waveformSize)] = fullrangesample;

        pushNextSampleIntoFifo(sample);
    }

    for (int channel = 0; channel < buffer.getNumChannels(); channel++)
    {
        auto* writePtr = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; sample++)
        {
            float Data = writePtr[sample];
     


            writePtr[sample] = Data;
        }
    }

}

//==============================================================================
bool Physical_Model_StringAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Physical_Model_StringAudioProcessor::createEditor()
{
    return new Physical_Model_StringAudioProcessorEditor (*this);
}

//==============================================================================
void Physical_Model_StringAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{

}

void Physical_Model_StringAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{

}

//==============================================================================
void Physical_Model_StringAudioProcessor::getChainSettings(ChainSettings& settings)
{
    //ADSR (
    settings.Attack = apvts.getRawParameterValue("Attack")->load();
    settings.Decay = apvts.getRawParameterValue("Decay")->load();
    settings.Sustain = apvts.getRawParameterValue("Sustain")->load();
    settings.Release = apvts.getRawParameterValue("Release")->load();

    settings.BridgeRefCoeff = apvts.getRawParameterValue("BRC")->load();
    settings.PluckPos = apvts.getRawParameterValue("PluckPos")->load();
}

juce::AudioProcessorValueTreeState::ParameterLayout
Physical_Model_StringAudioProcessor::createParameterLayout() 
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Attack", "Attack",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Decay", "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.8f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Release", "Release",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "BRC", "BRC",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        -1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "PluckPos", "PluckPos",
        juce::NormalisableRange<float>(0.2f, 1.0f, 0.01f),
        0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Physical_Model_StringAudioProcessor();
}
