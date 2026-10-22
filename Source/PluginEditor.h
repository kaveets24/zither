#pragma once

#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

/** Placeholder UI: the generic parameter list plus an on-screen keyboard for quick auditioning. */
class ZitherAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ZitherAudioProcessorEditor (ZitherAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::GenericAudioProcessorEditor parameterPanel;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZitherAudioProcessorEditor)
};
