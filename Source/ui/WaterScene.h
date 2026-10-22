#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace zither::ui
{
/**
    The underwater grotto behind everything. Deep water with falling light, and violet walls that slowly melt:
    ceiling drips swell and let go of droplets, and the side walls flow downward and pool at the floor.

    Time only moves when advance() is called, so the scene can be frozen as a still frame.
*/
class WaterScene final : public juce::Component
{
public:
    WaterScene();

    void advance (double deltaSeconds);
    void paint (juce::Graphics&) override;

private:
    struct Drip
    {
        float x = 0.5f;      // normalized position along the ceiling
        float width = 0.03f; // normalized half-width
        float reach = 40.0f; // longest stretch in px before a droplet lets go
        float rate = 0.05f;  // cycles per second
        float phase = 0.0f;
    };

    /** A run of melt sliding down a side wall: tapering tail above, rounded head below. */
    struct SlideDrip
    {
        float offset = 0.0f, speed = 8.0f, amplitude = 14.0f, tail = 90.0f, head = 14.0f;
    };

    struct Droplet
    {
        float x = 0.0f, y = 0.0f, radius = 0.0f, velocity = 0.0f, age = 0.0f;
        bool alive = false;
    };

    float dripLength (const Drip&) const noexcept;
    float ceilingEdge (float x, float width, float depth, float timeOffset) const noexcept;
    float wallThickness (float y, float width, float height, bool leftSide, int layer) const noexcept;
    void paintWater (juce::Graphics&, juce::Rectangle<float>) const;
    void paintLight (juce::Graphics&, juce::Rectangle<float>) const;
    void paintSideWall (juce::Graphics&, juce::Rectangle<float>, bool leftSide, int layer) const;
    void paintCeiling (juce::Graphics&, juce::Rectangle<float>, int layer) const;
    void paintDroplets (juce::Graphics&) const;
    void releaseDroplet (const Drip&);

    double time = 0.0;
    std::array<Drip, 9> drips;
    std::array<std::array<SlideDrip, 5>, 2> slideDrips;
    std::array<Droplet, 24> droplets;
    juce::Random random { 0x5eed };
};
} // namespace zither::ui
