#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Physical_Model_StringAudioProcessorEditor::Physical_Model_StringAudioProcessorEditor (Physical_Model_StringAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      midikeyboard(audioProcessor.getMidiKeyboardState(),
      juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(800, 300);

    addAndMakeVisible(midikeyboard);

    auto setupSlider = [this](juce::Slider& slider,
        juce::Label& label,
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,
        const juce::String& paramID,
        const juce::String& labelText)
        {
            addAndMakeVisible(slider);
            slider.setLookAndFeel(&sliderlookandfeel);
            slider.setSliderStyle(juce::Slider::LinearBarVertical);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0); 
            slider.setMouseCursor(juce::MouseCursor::DraggingHandCursor);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts, paramID, slider);

            addAndMakeVisible(label);
            label.setText(labelText, juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centred);
            label.attachToComponent(&slider, false); 
        };

    //==============================================================================
    // Create each slider with matching parameter IDs
    setupSlider(AttackSlider, AttackLabel, AttackSliderAttachment, "Attack", "A");
    setupSlider(DecaySider, DecayLabel, DecaySliderAttachment, "Decay", "D");
    setupSlider(SustainSlider, SustainLabel, SustainSliderAttachment, "Sustain", "S");
    setupSlider(ReleaseSlider, ReleaseLabel, ReleaseSliderAttachment, "Release", "R");
    setupSlider(BRCSlider, BRCLabel, BRCSliderAttachment, "BRC", "BRC");
    setupSlider(PluckPosSlider, PluckPosLabel, PluckPosSliderAttachment, "PluckPos", "Pos");

    //Visualiser Toggle
    VisualiserSwitchButton.setLookAndFeel(&roundedbuttonlookandfeel);
    VisualiserSwitchButton.setButtonText("~");
    VisualiserSwitchButton.setMouseCursor(MouseCursor::PointingHandCursor);
    addAndMakeVisible(VisualiserSwitchButton);

    pVisualiserSwitchButton->onClick = [this] {
        VisualiserTypeToggle = !VisualiserTypeToggle;
        DBG("Visualiser = " << (VisualiserTypeToggle ? "Waveform" : "Spectrum"));
        VisualiserTypeToggle ? VisualiserSwitchButton.setButtonText("~") : VisualiserSwitchButton.setButtonText("|||");
        };

    VisualiserTypeToggle ? VisualiserSwitchButton.setButtonText("~") : VisualiserSwitchButton.setButtonText("|||");

    startTimerHz(60);
}

Physical_Model_StringAudioProcessorEditor::~Physical_Model_StringAudioProcessorEditor()
{
    AttackSlider.setLookAndFeel(nullptr);
    DecaySider.setLookAndFeel(nullptr);
    SustainSlider.setLookAndFeel(nullptr);
    ReleaseSlider.setLookAndFeel(nullptr);
    BRCSlider.setLookAndFeel(nullptr);
    PluckPosSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void Physical_Model_StringAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Base gradient background
    juce::ColourGradient gradient(
        juce::Colours::black, 0.0f, 0.0f,
        juce::Colours::black, (float)getWidth(), (float)getHeight(), false);
    g.setGradientFill(gradient);
    g.fillRect(getLocalBounds());

    float cornerRadius = 0.0f;
    float outlinethickness = 3.0f;

    juce::Rectangle<float> bounds = getLocalBounds().toFloat().reduced(outlinethickness * 0.5f);
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(bounds, cornerRadius, outlinethickness);

    g.setFont(juce::Font(15.0f));

    float originalLineY = cornerRadius + outlinethickness * 53.5f;
    g.setColour(juce::Colours::white);
    g.fillRect(0.0f,
        originalLineY - (outlinethickness / 2.0f),
        (float)getWidth(),
        outlinethickness);

    float boxMargin = 2.0f;
    float boxX = bounds.getX() + boxMargin;
    float boxY = bounds.getY() + boxMargin;
    float boxWidth = 300.0f;
    float boxHeight = originalLineY - boxY; 

    juce::Rectangle<float> topLeftBox(boxX, boxY, boxWidth, boxHeight);
    g.setColour(juce::Colours::black);
    g.fillRect(topLeftBox);

    g.setColour(juce::Colours::white);
    g.fillRect(topLeftBox.getRight() - (outlinethickness / 2.0f),
        topLeftBox.getY(),
        outlinethickness,
        topLeftBox.getHeight());

    if (VisualiserTypeToggle == true) {
        drawFrame(g, topLeftBox);
    }
    else {
        drawWavePeriod(g, topLeftBox);
    } 
}

void Physical_Model_StringAudioProcessorEditor::resized()
{
    const int margin = 1;
    const int keyboardHeight = 136;

    const float outlinethickness = 3.0f;
    juce::Rectangle<int> boxBounds = getLocalBounds().reduced((int)(outlinethickness * 0.5f));

    // === Layout constants ===
    const float cornerRadius = 0.0f;
    const float originalLineY = cornerRadius + outlinethickness * 53.5f;

    const float boxMargin = 2.0f;
    const float boxWidth = 300.0f;

    // The black box area (top-left)
    const float boxX = boxMargin;
    const float boxY = boxMargin;

    // === Slider area ===
    const float sliderAreaX = boxX + boxWidth + 20.0f;  // spacing from the black box
    const float baseSliderAreaY = boxY + 10.0f;        // original top margin
    const float extraVerticalOffset = 23.0f;           // <<--- move sliders down by this many pixels
    const float sliderAreaY = baseSliderAreaY + extraVerticalOffset;

    const float sliderAreaWidth = (float)getWidth() - sliderAreaX - 20.0f;
    const float sliderAreaHeight = originalLineY - 20.0f - extraVerticalOffset; // keep above the horizontal line

    // === Slider dimensions ===
    const int numSliders = 6;
    const float gap = 8.0f; // spacing between sliders
    const float availableWidth = sliderAreaWidth - (numSliders - 1) * gap;
    const float sliderSlotWidth = availableWidth / (float)numSliders;

    const int labelHeight = 20;
    const float sliderHeight = juce::jmax(20.0f, sliderAreaHeight - (float)labelHeight); // ensure positive

    // Helper to set bounds: label BELOW slider
    auto setSliderBounds = [&](juce::Slider& slider, juce::Label& label, int index)
        {
            float x = sliderAreaX + index * (sliderSlotWidth + gap);
            int sx = (int)std::round(x + (sliderSlotWidth - sliderSlotWidth * 0.6f) * 0.5f); // center narrower slider
            int sWidth = (int)std::round(sliderSlotWidth * 0.6f);
            int sHeight = (int)std::round(sliderHeight);
            int sY = (int)std::round(sliderAreaY);

            slider.setBounds(sx, sY, sWidth, sHeight);

            // Label below the slider, centered in the slot
            int labelX = (int)std::round(x);
            int labelW = (int)std::round(sliderSlotWidth);
            int labelY = sY + sHeight + 4; // small gap between slider and label
            label.setBounds(labelX, labelY, labelW, labelHeight);
        };

    setSliderBounds(AttackSlider, AttackLabel, 0);
    setSliderBounds(DecaySider, DecayLabel, 1);  // keep your variable name
    setSliderBounds(SustainSlider, SustainLabel, 2);
    setSliderBounds(ReleaseSlider, ReleaseLabel, 3);
    setSliderBounds(BRCSlider, BRCLabel, 4);
    setSliderBounds(PluckPosSlider, PluckPosLabel, 5);

    // Position the keyboard flush with the bottom of the background box
    midikeyboard.setBounds(
        boxBounds.getX() + margin,
        boxBounds.getBottom() - keyboardHeight - margin,
        boxBounds.getWidth() - 2 * margin,
        keyboardHeight
    );

    //VisualiserToggle
    VisualiserSwitchButton.setBounds(10, 10, 30, 30);
}


void Physical_Model_StringAudioProcessorEditor::drawNextFrameOfSpectrum()
{
    auto fftdata = audioProcessor.fftData.data();

    audioProcessor.window.multiplyWithWindowingTable(audioProcessor.fftData.data(), audioProcessor.fftSize);

    audioProcessor.forwardFFT.performFrequencyOnlyForwardTransform(fftdata);
    auto mindB = -100.0f;
    auto maxdB = 0.0f;
    for (int i = 0; i < audioProcessor.scopeSize; ++i)
    {
        auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)audioProcessor.scopeSize) * 0.2f);
        auto fftDataIndex = juce::jlimit(0, audioProcessor.fftSize / 2, (int)(skewedProportionX * (float)audioProcessor.fftSize * 0.5f));
        auto Level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftdata[fftDataIndex]) - juce::Decibels::gainToDecibels((float)audioProcessor.fftSize)),
            mindB,
            maxdB,
            0.0f,
            1.0f);
        audioProcessor.scopeData[i] = Level;
    }
}

void Physical_Model_StringAudioProcessorEditor::drawFrame(juce::Graphics& g, juce::Rectangle<float> box)
{
    int width = (int)box.getWidth();
    int height = (int)box.getHeight();

    juce::Path spectrumPath;
    spectrumPath.startNewSubPath(box.getX(), box.getBottom()); // start at bottom-left of the box

    for (int i = 0; i < audioProcessor.scopeSize; ++i)
    {
        float x = juce::jmap((float)i, 0.0f, (float)(audioProcessor.scopeSize - 1),
            box.getX(), box.getRight());
        float y = juce::jmap(audioProcessor.scopeData[i], 0.0f, 1.0f,
            box.getBottom(), box.getY()); // invert y
        spectrumPath.lineTo(x, y);
    }

    spectrumPath.lineTo(box.getRight(), box.getBottom());
    spectrumPath.closeSubPath();

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.fillPath(spectrumPath);

    g.setColour(juce::Colours::white);
    g.strokePath(spectrumPath, juce::PathStrokeType(1.0f));
}

void Physical_Model_StringAudioProcessorEditor::drawWavePeriod(juce::Graphics& g, juce::Rectangle<float> box)
{
    int width = (int)box.getWidth();
    int height = (int)box.getHeight();

    juce::Path path;
    path.startNewSubPath(box.getX(), box.getBottom());

    auto& buffer = audioProcessor.waveformBuffer;
    int writeIndex = audioProcessor.waveformWriteIndex.load();

    for (int i = 0; i < audioProcessor.waveformSize; ++i)
    {
        int idx = (writeIndex + i) % audioProcessor.waveformSize;
        float sample = buffer[idx];

        float x = juce::jmap((float)i, 0.0f, (float)(audioProcessor.waveformSize - 1),
            box.getX(), box.getRight());
        float y = juce::jmap(sample, -1.0f, 1.0f,
            box.getBottom(), box.getY());
        path.lineTo(x, y);
    }

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}


void Physical_Model_StringAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.nextFFTBlockReady)
    {
        drawNextFrameOfSpectrum();
        audioProcessor.nextFFTBlockReady = false;
        repaint();
    }
}