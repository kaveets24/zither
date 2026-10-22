#pragma once

#include "../PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace zither::ui
{
/**
    The strings, drawn across the water. They vibrate with each string's real level, send out ripples
    from where they were struck, and release bubbles. Dragging across them strums: the hand is the strummer.

    With animation off they stay as still lines, and they can still be played.
*/
class StringField final : public juce::Component
{
public:
    explicit StringField (ZitherAudioProcessor&);

    /** Called from the editor's timer: polls the processor and advances the animation. */
    void update (double deltaSeconds, bool animate);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    static constexpr int kMaxStrings = Tuning::kMaxStrings;

    struct StringVisual
    {
        float level = 0.0f;
        float phase = 0.0f;
        uint32_t seenStrikes = 0;
        float touchX = -1.0f;
        double touchTime = 0.0;
    };

    struct Ripple
    {
        float x = 0.0f, y = 0.0f, age = 0.0f, strength = 0.0f;
        bool alive = false;
    };

    struct Bubble
    {
        float x = 0.0f, y = 0.0f, radius = 0.0f, rise = 0.0f, sway = 0.0f, phase = 0.0f, age = 0.0f, life = 0.0f;
        bool alive = false;
    };

    juce::Rectangle<float> stringArea() const;
    float stringY (int index) const;
    float stringSpacing() const;
    int stringNear (float y, float tolerance) const;
    void refreshTuning();
    void strike (int index, float x, float velocity);
    void spawnEffects (int index, float x, float strength);
    void spawnBubble (float x, float y, float radius, float rise);

    ZitherAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& parameters;
    Tuning tuning;
    TuningSettings tuningSettings;
    bool animating = false;

    std::array<StringVisual, kMaxStrings> strings;
    std::array<Ripple, 48> ripples;
    std::array<Bubble, 160> bubbles;
    double clock = 0.0;
    float ambientTimer = 0.0f;
    juce::Random random { 0xb0bb1e };

    juce::Point<float> lastDrag;
    double lastDragTime = 0.0;
};
} // namespace zither::ui
