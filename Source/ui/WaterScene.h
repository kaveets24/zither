#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zither::ui
{
/**
    The open water behind everything. Above, the rippling underside of the surface lets light fall through in
    shafts and caustic bands. Below, layered swells roll slowly past and fade into the deep.

    Time only moves when advance() is called, so the scene can be frozen as a still frame.
*/
class WaterScene final : public juce::Component
{
public:
    WaterScene();

    void advance (double deltaSeconds);
    void paint (juce::Graphics&) override;

    /** A travelling wave line. Two summed sines at a non-integer ratio, so the crests never quite repeat. */
    struct Wave
    {
        float offset;    // baseline in px from the edge the wave hangs from
        float amplitude; // px
        float frequency; // radians per px
        float speed;     // radians per second; the sign sets the direction of travel
        float phase;
    };

private:
    float waveY (const Wave&, float x, float baseline) const noexcept;
    juce::Path waveLine (const Wave&, float width, float baseline) const;
    static juce::Path closedTo (juce::Path line, float width, float y);

    void paintWater (juce::Graphics&, juce::Rectangle<float>) const;
    void paintSurface (juce::Graphics&, juce::Rectangle<float>) const;
    void paintLight (juce::Graphics&, juce::Rectangle<float>) const;
    void paintSwells (juce::Graphics&, juce::Rectangle<float>) const;
    void paintVignette (juce::Graphics&, juce::Rectangle<float>) const;

    double time = 0.0;
};
} // namespace zither::ui
