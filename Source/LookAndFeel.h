/*
  ==============================================================================

    LookAndFeel.h
    Created: 19 Oct 2025 3:35:29pm
    Author:  josep

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class WhiteSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WhiteSliderLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::trackColourId, juce::Colours::white);
    }

    //==============================================================
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // --- Track ---
        float trackX = x + width * 0.4f;
        float trackWidth = width * 0.2f;
        juce::Rectangle<float> trackBounds(trackX, (float)y, trackWidth, (float)height);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(trackBounds);

        // --- Filled Track (active) ---
        juce::Rectangle<float> fill(trackBounds);
        fill.setY(sliderPos);
        fill.setHeight(trackBounds.getBottom() - sliderPos);
        g.setColour(juce::Colours::white);
        g.fillRect(fill);

        // --- Thumb ---
        const float maxThumbRadius = 12.0f;                 // safe max radius
        const float thumbRadius = juce::jmin(width * 0.3f, maxThumbRadius);
        const float cx = x + width * 0.5f;
        const float cy = juce::jlimit(y + thumbRadius, y + height - thumbRadius, sliderPos);

        // Outer white circle
        g.setColour(juce::Colours::white);
        g.fillEllipse(cx - thumbRadius, cy - thumbRadius, thumbRadius * 2.0f, thumbRadius * 2.0f);

        // Inner black circle
        const float innerRadius = thumbRadius * 0.9f;
        g.setColour(juce::Colours::black);
        g.fillEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
    }

    //==============================================================
    int getSliderThumbRadius(juce::Slider&) override
    {
        return 12; // matches maxThumbRadius above
    }
};


class ButtonLookandFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& /*backgroundColour*/,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);

        // Fill green background if pressed
        if (isButtonDown)
            g.setColour(juce::Colours::green);
        else
            g.setColour(juce::Colours::transparentBlack);

        g.fillRect(bounds);

        // Overlay transparent grey if hovered
        if (isMouseOverButton)
        {
            g.setColour(juce::Colours::grey.withAlpha(0.3f));
            g.fillRect(bounds);
        }

        // White border
        g.setColour(juce::Colours::white);
        g.drawRect(bounds, 3.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
        bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        auto bounds = button.getLocalBounds();
        g.setColour(juce::Colours::white);
        g.setFont(15.0f);
        g.drawFittedText(button.getButtonText(), bounds, juce::Justification::centred, 1);
    }
};

class RoundedButtonLookandFeel : public ButtonLookandFeel
{
private:

    Colour* myCol;

public:

    void getColour(Colour* mycol) { myCol = mycol; }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& /*backgroundColour*/,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
        float cornerRadius = 5.0f; // You can adjust this for more or less roundness

        // Fill green background if pressed
        if (isButtonDown)
            g.setColour(juce::Colours::green.withAlpha(0.4f));
        else
            g.setColour(juce::Colours::transparentBlack);

        g.fillRoundedRectangle(bounds, cornerRadius);

        // Overlay transparent grey if hovered
        if (isMouseOverButton)
        {
            g.setColour(juce::Colours::grey.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds, cornerRadius);
        }

        // White border
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
    }
};