#pragma once

#include "PluginProcessor.h"
#include "ui/AquaLookAndFeel.h"
#include "ui/ControlPanel.h"
#include "ui/StringField.h"
#include "ui/WaterScene.h"

#include <juce_gui_basics/juce_gui_basics.h>

/**
    Open water under a rippling surface, the strings hanging in the water, and one glass panel of controls.
    One 30 Hz timer drives everything. With visuals off, the world freezes into a still frame.
*/
class ZitherAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit ZitherAudioProcessorEditor (ZitherAudioProcessor&);
    ~ZitherAudioProcessorEditor() override;

    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setVisualsEnabled (bool);
    juce::String tuningSummary() const;

    ZitherAudioProcessor& zitherProcessor;
    zither::ui::AquaLookAndFeel lookAndFeel;

    zither::ui::WaterScene scene;
    zither::ui::StringField stringField;
    zither::ui::ControlPanel controls;
    juce::ToggleButton visualsToggle { "visuals" };

    juce::String summary;
    double lastTick = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZitherAudioProcessorEditor)
};
