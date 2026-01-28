#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
class Physical_Model_StringAudioProcessorEditor  : public juce::AudioProcessorEditor, 
                                                   private juce::Timer
{
public:
    Physical_Model_StringAudioProcessorEditor (Physical_Model_StringAudioProcessor&);
    ~Physical_Model_StringAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void drawNextFrameOfSpectrum();
    void drawFrame(juce::Graphics& g, juce::Rectangle<float> box);
    void drawWavePeriod(juce::Graphics& g, juce::Rectangle<float> box);

private:

    std::vector<juce::Point<int>> starPositions;
    int numStars = 300;

    double startTime{ 0 };

    //Visualiser Toggle stuff
    RoundedButtonLookandFeel roundedbuttonlookandfeel;

    TextButton VisualiserSwitchButton;
    TextButton* pVisualiserSwitchButton = &VisualiserSwitchButton;

    bool VisualiserTypeToggle = false;

    Physical_Model_StringAudioProcessor& audioProcessor;

    MidiKeyboardComponent midikeyboard;

    //Sliders
    Slider AttackSlider, DecaySider, SustainSlider, ReleaseSlider, BRCSlider, PluckPosSlider;

    //Slider Attachment
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>

        //ADSR
        AttackSliderAttachment, DecaySliderAttachment, SustainSliderAttachment, ReleaseSliderAttachment,

        //Others
        BRCSliderAttachment, PluckPosSliderAttachment;

    //SliderLabels
    Label AttackLabel, DecayLabel, SustainLabel, ReleaseLabel,
          BRCLabel, PluckPosLabel;

    WhiteSliderLookAndFeel sliderlookandfeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Physical_Model_StringAudioProcessorEditor)
};
