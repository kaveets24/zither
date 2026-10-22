#include "PluginEditor.h"

namespace
{
constexpr int kHeaderHeight = 56;
constexpr int kKeyboardHeight = 80;
} // namespace

ZitherAudioProcessorEditor::ZitherAudioProcessorEditor (ZitherAudioProcessor& p)
    : AudioProcessorEditor (p),
      parameterPanel (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (parameterPanel);

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (480, 420, 1600, 1600);
    setSize (640, 780);
}

void ZitherAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1440), 0.0f, 0.0f,
                                             juce::Colour (0xff0f3b3a), bounds.getWidth(), bounds.getHeight(), false));
    g.fillAll();

    auto header = getLocalBounds().removeFromTop (kHeaderHeight).reduced (16, 8);
    g.setColour (juce::Colour (0xfff3e6c4));
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("ZITHER", header.removeFromLeft (140), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffb9d7d0));
    g.setFont (juce::FontOptions (14.0f));
    g.drawText ("open-tuned waveguide zither", header, juce::Justification::centredRight);
}

void ZitherAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (kHeaderHeight);
    keyboard.setBounds (area.removeFromBottom (kKeyboardHeight));
    parameterPanel.setBounds (area.reduced (8, 0));
}
