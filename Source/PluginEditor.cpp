#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/Palette.h"

namespace
{
constexpr int kHeaderHeight = 48;
constexpr int kPanelHeight = 150;
constexpr int kPanelMargin = 16;

const char* const noteNames[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };
} // namespace

ZitherAudioProcessorEditor::ZitherAudioProcessorEditor (ZitherAudioProcessor& p)
    : AudioProcessorEditor (p),
      zitherProcessor (p),
      stringField (p),
      controls (p.getParameters())
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (scene);
    addAndMakeVisible (stringField);
    addAndMakeVisible (controls);
    addAndMakeVisible (visualsToggle);

    const bool visuals = zitherProcessor.getParameters().state.getProperty (ZitherAudioProcessor::visualsProperty, true);
    visualsToggle.setToggleState (visuals, juce::dontSendNotification);
    visualsToggle.onClick = [this] { setVisualsEnabled (visualsToggle.getToggleState()); };

    summary = tuningSummary();
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);

    setResizable (true, true);
    setResizeLimits (760, 520, 1800, 1200);
    setSize (960, 640);
}

ZitherAudioProcessorEditor::~ZitherAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ZitherAudioProcessorEditor::setVisualsEnabled (bool enabled)
{
    zitherProcessor.getParameters().state.setProperty (ZitherAudioProcessor::visualsProperty, enabled, nullptr);
    scene.repaint();
}

juce::String ZitherAudioProcessorEditor::tuningSummary() const
{
    auto& state = zitherProcessor.getParameters();
    auto index = [&state] (const char* id) { return static_cast<int> (state.getRawParameterValue (id)->load()); };

    const auto mode = juce::String (zither::getModeName (static_cast<zither::Mode> (index (zither::param::mode)))).toLowerCase();
    zither::Tuning tuning;
    tuning.set ({ index (zither::param::root), static_cast<zither::Mode> (index (zither::param::mode)),
                  index (zither::param::numStrings), index (zither::param::lowestOctave) });

    return juce::String (noteNames[juce::jlimit (0, 11, index (zither::param::root))]) + " " + mode
         + "  \xc2\xb7  " + juce::String (tuning.getNumStrings()) + " strings";
}

void ZitherAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.0, 0.1, (now - lastTick) * 0.001);
    lastTick = now;

    const bool animate = visualsToggle.getToggleState();
    if (animate)
    {
        scene.advance (dt);
        scene.repaint();
    }

    stringField.update (dt, animate);

    if (const auto latest = tuningSummary(); latest != summary)
    {
        summary = latest;
        repaint (getLocalBounds().removeFromTop (kHeaderHeight));
    }
}

void ZitherAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    // Header text floats over the melting ceiling.
    auto header = getLocalBounds().removeFromTop (kHeaderHeight).reduced (24, 0);

    g.setColour (zither::ui::palette::pearl.withAlpha (0.92f));
    g.setFont (zither::ui::palette::font (20.0f, 0.45f));
    g.drawText ("zither", header.removeFromLeft (120), juce::Justification::centredLeft);

    g.setColour (zither::ui::palette::mist.withAlpha (0.75f));
    g.setFont (zither::ui::palette::font (12.0f, 0.18f));
    g.drawText (summary, header.withTrimmedRight (120), juce::Justification::centredLeft);
}

void ZitherAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    scene.setBounds (area);

    auto header = area.removeFromTop (kHeaderHeight);
    visualsToggle.setBounds (header.removeFromRight (130).reduced (16, 12));

    auto panel = area.removeFromBottom (kPanelHeight + kPanelMargin);
    const int panelWidth = juce::jmin (panel.getWidth() - 2 * kPanelMargin, 820);
    controls.setBounds (panel.withSizeKeepingCentre (panelWidth, kPanelHeight).translated (0, -kPanelMargin / 2));

    stringField.setBounds (area);
}
