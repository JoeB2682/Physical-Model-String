/*
  ==============================================================================

    SynthSound.h
    Created: 19 Oct 2025 3:36:29pm
    Author:  josep

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class SynthSound : public SynthesiserSound 
{
public:

    bool appliesToNote(int midiNoteNumber) { return true; }
    bool appliesToChannel(int midiChannels) { return true; }
};