#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"
#include "SynthVoice.h"
#include <stk_wrapper/stk_wrapper.h>

//==============================================================================
class Physical_Model_StringAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Physical_Model_StringAudioProcessor();
    ~Physical_Model_StringAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void getChainSettings(ChainSettings& settings);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts{
        *this, nullptr, "Parameters", createParameterLayout()
    };


    //==============================================================================
    juce::MidiKeyboardState& getMidiKeyboardState() { return midiKeyboardState; }

private:

    Synthesiser mySynth;
    SynthVoice* myVoice;

    juce::MidiKeyboardState midiKeyboardState;

    ChainSettings processorChainsettings;

    double lastSampleRate;

    float mix = 0.0f, pan = 0.50;

  public:

    static constexpr auto fftOrder = 11;
    static constexpr auto fftSize = 1 << fftOrder;

    void pushNextSampleIntoFifo(float sample);
    bool getNextFFTBlockReady() const { return nextFFTBlockReady; }
    void setNextFFTBlockReady(bool ready) { nextFFTBlockReady = ready; }
    const float* getFFTData() const { return fftData.data(); }

    int scopeSize = 1024;
    float scopeData[1024];

    juce::dsp::WindowingFunction<float> window;

    juce::dsp::FFT forwardFFT;
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    static constexpr int waveformSize = 512;
    std::array<float, waveformSize> waveformBuffer{};
    std::atomic<int> waveformWriteIndex{ 0 };

    juce::AudioBuffer<float> timeDomainBuffer;

    int numSamples{};
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Physical_Model_StringAudioProcessor)
};
