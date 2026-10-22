#include "AquaLookAndFeel.h"
#include "Palette.h"

namespace zither::ui
{
AquaLookAndFeel::AquaLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, palette::pearl);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::PopupMenu::backgroundColourId, palette::deepWater.withAlpha (0.97f));
    setColour (juce::PopupMenu::textColourId, palette::pearl.withAlpha (0.8f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, palette::aqua.withAlpha (0.14f));
    setColour (juce::PopupMenu::highlightedTextColourId, palette::pearl);
    setColour (juce::ToggleButton::textColourId, palette::mist);
}

void AquaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float emphasis = slider.isMouseOverOrDragging() ? 1.0f : 0.75f;

    // A faint pearl "bubble" at the core.
    g.setGradientFill (juce::ColourGradient (palette::pearl.withAlpha (0.10f * emphasis), centre.x, centre.y - radius * 0.3f,
                                             juce::Colours::transparentWhite, centre.x, centre.y + radius, true));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (palette::pearl.withAlpha (0.12f));
    g.strokePath (track, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (palette::aqua.withAlpha (0.16f * emphasis));
    g.strokePath (value, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (palette::aqua.withAlpha (0.9f * emphasis));
    g.strokePath (value, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto tip = centre.getPointOnCircumference (radius, angle);
    g.setColour (palette::pearl.withAlpha (emphasis));
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tip));
}

void AquaLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const float alpha = box.isMouseOver (true) ? 0.35f : 0.18f;
    const float lineY = static_cast<float> (height) - 3.0f;

    g.setColour (palette::pearl.withAlpha (alpha));
    g.drawLine (8.0f, lineY, static_cast<float> (width) - 8.0f, lineY, 1.0f);

    // A small chevron, like a fin.
    const float cx = static_cast<float> (width) - 12.0f;
    const float cy = static_cast<float> (height) * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 3.0f, cy - 1.5f);
    chevron.lineTo (cx, cy + 1.5f);
    chevron.lineTo (cx + 3.0f, cy - 1.5f);
    g.setColour (palette::mist.withAlpha (alpha + 0.3f));
    g.strokePath (chevron, juce::PathStrokeType (1.2f));
}

juce::Font AquaLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return palette::font (14.0f, 0.05f);
}

void AquaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 20, box.getHeight() - 4);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centred);
}

void AquaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (palette::aqua.withAlpha (0.15f));
    g.drawRect (0, 0, width, height);
}

juce::Font AquaLookAndFeel::getPopupMenuFont()
{
    return palette::font (14.0f, 0.05f);
}

void AquaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool isHighlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();
    const bool hasText = button.getButtonText().isNotEmpty();

    auto pill = juce::Rectangle<float> (32.0f, 16.0f);
    pill = hasText ? pill.withPosition (bounds.getRight() - 34.0f, bounds.getCentreY() - 8.0f)
                   : pill.withCentre (bounds.getCentre());

    g.setColour (on ? palette::aqua.withAlpha (isHighlighted ? 0.55f : 0.42f)
                    : palette::pearl.withAlpha (isHighlighted ? 0.16f : 0.1f));
    g.fillRoundedRectangle (pill, 8.0f);

    const auto knob = juce::Rectangle<float> (12.0f, 12.0f)
                          .withCentre ({ on ? pill.getRight() - 8.0f : pill.getX() + 8.0f, pill.getCentreY() });
    g.setColour (on ? palette::pearl : palette::mist.withAlpha (0.7f));
    g.fillEllipse (knob);

    if (hasText)
    {
        g.setColour (findColour (juce::ToggleButton::textColourId).withAlpha (on ? 0.95f : 0.6f));
        g.setFont (palette::font (12.0f, 0.2f));
        g.drawText (button.getButtonText(), bounds.withRight (pill.getX() - 8.0f), juce::Justification::centredRight);
    }
}
} // namespace zither::ui
