#include "WaterScene.h"
#include "Palette.h"

#include <cmath>
#include <utility>

namespace zither::ui
{
namespace
{
    constexpr float kCeilingDepth = 0.085f; // ceiling base as a fraction of height

    float smoothstep (float t) noexcept
    {
        t = juce::jlimit (0.0f, 1.0f, t);
        return t * t * (3.0f - 2.0f * t);
    }
} // namespace

WaterScene::WaterScene()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, false);

    for (size_t i = 0; i < drips.size(); ++i)
    {
        auto& d = drips[i];
        d.x = (static_cast<float> (i) + 0.5f + 0.6f * (random.nextFloat() - 0.5f)) / static_cast<float> (drips.size());
        d.width = 0.012f + 0.02f * random.nextFloat();
        d.reach = 22.0f + 48.0f * random.nextFloat();
        d.rate = 0.025f + 0.05f * random.nextFloat();
        d.phase = random.nextFloat();
    }

    for (auto& side : slideDrips)
    {
        for (auto& d : side)
        {
            d.offset = 900.0f * random.nextFloat();
            d.speed = 5.0f + 9.0f * random.nextFloat();
            d.amplitude = 9.0f + 16.0f * random.nextFloat();
            d.tail = 60.0f + 120.0f * random.nextFloat();
            d.head = d.amplitude * (1.0f + 0.4f * random.nextFloat()); // round heads, not ledges
        }
    }
}

void WaterScene::advance (double deltaSeconds)
{
    const auto dt = static_cast<float> (deltaSeconds);
    time += deltaSeconds;

    for (auto& d : drips)
    {
        d.phase += d.rate * dt;
        if (d.phase >= 1.0f)
        {
            releaseDroplet (d);
            d.phase -= 1.0f;
        }
    }

    // Melted wax is denser than water: droplets sink slowly and dissolve.
    for (auto& drop : droplets)
    {
        if (! drop.alive)
            continue;

        drop.age += dt;
        drop.velocity = juce::jmin (drop.velocity + 14.0f * dt, 34.0f);
        drop.y += drop.velocity * dt;
        drop.alive = drop.age < 7.0f && drop.y < static_cast<float> (getHeight());
    }
}

float WaterScene::dripLength (const Drip& d) const noexcept
{
    // Never fully retracts: when the bulb lets go, the drip snaps back to a stub and starts swelling again.
    return d.reach * (0.3f + 0.7f * smoothstep (d.phase));
}

void WaterScene::releaseDroplet (const Drip& d)
{
    for (auto& drop : droplets)
    {
        if (drop.alive)
            continue;

        const float width = static_cast<float> (getWidth());
        drop.x = d.x * width;
        drop.radius = 2.0f + d.width * 120.0f;
        drop.y = ceilingEdge (drop.x, width, kCeilingDepth * static_cast<float> (getHeight()), 0.0f) + drop.radius;
        drop.velocity = 2.0f;
        drop.age = 0.0f;
        drop.alive = true;
        return;
    }
}

float WaterScene::ceilingEdge (float x, float width, float depth, float timeOffset) const noexcept
{
    const auto t = static_cast<float> (time) + timeOffset;
    float y = depth + 6.0f * std::sin (x * 0.011f + t * 0.13f) + 4.0f * std::sin (x * 0.029f - t * 0.09f + 1.7f);

    for (const auto& d : drips)
    {
        const float u = std::abs (x / width - d.x) / (d.width * (1.0f - 0.3f * d.phase));
        if (u < 1.0f)
            y += dripLength (d) * std::pow (std::cos (u * juce::MathConstants<float>::halfPi), 2.0f);
    }

    return y;
}

void WaterScene::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    paintWater (g, bounds);
    paintLight (g, bounds);

    for (int layer = 1; layer >= 0; --layer)
    {
        paintSideWall (g, bounds, true, layer);
        paintSideWall (g, bounds, false, layer);
        paintCeiling (g, bounds, layer);
    }

    paintDroplets (g);
}

void WaterScene::paintWater (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setGradientFill (juce::ColourGradient::vertical (palette::deepWater, bounds.getY(), palette::abyss, bounds.getBottom()));
    g.fillRect (bounds);

    // A soft glow in the middle of the water, where the strings hang.
    g.setGradientFill (juce::ColourGradient (palette::lagoon.withAlpha (0.55f), bounds.getCentreX(), bounds.getHeight() * 0.45f,
                                             juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getBottom(), true));
    g.fillRect (bounds);
}

void WaterScene::paintLight (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const auto t = static_cast<float> (time);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Shafts of light falling from the surface, swaying slowly.
    for (int i = 0; i < 3; ++i)
    {
        const float fi = static_cast<float> (i);
        const float top = w * (0.28f + 0.22f * fi) + 30.0f * std::sin (t * 0.05f + fi * 2.1f);
        const float spread = w * (0.035f + 0.012f * fi);
        const float lean = w * 0.08f * std::sin (t * 0.03f + fi);

        juce::Path ray;
        ray.startNewSubPath (top - spread * 0.5f, 0.0f);
        ray.lineTo (top + spread * 0.5f, 0.0f);
        ray.lineTo (top + lean + spread * 1.6f, h * 0.85f);
        ray.lineTo (top + lean - spread * 1.6f, h * 0.85f);
        ray.closeSubPath();

        const float shimmer = 0.045f + 0.02f * std::sin (t * 0.21f + fi * 1.3f);
        g.setGradientFill (juce::ColourGradient::vertical (palette::aqua.withAlpha (shimmer), 0.0f,
                                                           juce::Colours::transparentBlack, h * 0.85f));
        g.fillPath (ray);
    }

    // Caustic bands: light refracted by surface waves.
    for (int i = 0; i < 4; ++i)
    {
        const float fi = static_cast<float> (i);
        const float baseY = h * (0.2f + 0.17f * fi);

        juce::Path band;
        bool first = true;
        for (float x = 0.0f; x <= w; x += 12.0f)
        {
            const float y = baseY + 7.0f * std::sin (x * 0.009f + t * (0.17f + 0.03f * fi) + fi)
                                  + 4.0f * std::sin (x * 0.023f - t * 0.11f + fi * 2.0f);
            if (std::exchange (first, false))
                band.startNewSubPath (x, y);
            else
                band.lineTo (x, y);
        }

        g.setColour (palette::aqua.withAlpha (0.022f));
        g.strokePath (band, juce::PathStrokeType (26.0f, juce::PathStrokeType::curved));
        g.setColour (palette::pearl.withAlpha (0.025f));
        g.strokePath (band, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
    }
}

float WaterScene::wallThickness (float y, float w, float h, bool leftSide, int layer) const noexcept
{
    const auto t = static_cast<float> (time);
    const float side = leftSide ? 0.0f : 1.7f; // the two walls melt out of phase
    const float n = y / h;

    if (layer == 1)
    {
        // The recess behind: broad, slow swells.
        return w * 0.07f + 16.0f * std::sin (y * 0.007f - t * 0.06f + side * 3.0f)
             + 9.0f * std::sin (y * 0.019f - t * 0.1f + side) + w * 0.12f * n * n * n;
    }

    // Thickness grows toward the floor (melt pools there), with a slow surface wobble...
    float thickness = w * 0.03f
                    + 5.0f * std::sin (y * 0.013f - t * 0.25f + side)
                    + 3.0f * std::sin (y * 0.037f - t * 0.4f + side * 2.3f)
                    + w * 0.075f * n * n * n;

    // ...and runs of melt sliding down the face.
    for (const auto& d : slideDrips[leftSide ? 0 : 1])
    {
        const float cycle = h + d.tail + 2.0f * d.head;
        const float headY = std::fmod (d.offset + d.speed * t, cycle) - d.head;
        const float dy = y - headY;

        if (dy > -d.tail && dy < 0.0f)
        {
            const float u = (dy + d.tail) / d.tail;
            thickness += d.amplitude * u * u;
        }
        else if (dy >= 0.0f && dy < d.head)
        {
            const float u = dy / d.head;
            thickness += d.amplitude * std::sqrt (1.0f - u * u);
        }
    }

    return thickness;
}

void WaterScene::paintSideWall (juce::Graphics& g, juce::Rectangle<float> bounds, bool leftSide, int layer) const
{
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float edgeX = leftSide ? 0.0f : w;
    auto toX = [&] (float thickness) { return leftSide ? thickness : w - thickness; };

    juce::Path wall, rim;
    wall.startNewSubPath (edgeX, 0.0f);
    for (float y = 0.0f; y <= h + 8.0f; y += 5.0f)
    {
        const float x = toX (wallThickness (y, w, h, leftSide, layer));
        wall.lineTo (x, y);
        if (y < 1.0f)
            rim.startNewSubPath (x, y);
        else
            rim.lineTo (x, y);
    }
    wall.lineTo (edgeX, h + 8.0f);
    wall.closeSubPath();

    if (layer == 1)
    {
        // A shadowed recess that fades into the water, so it reads as depth rather than a frame.
        const float span = w * 0.16f;
        const auto shade = palette::melt.darker (0.8f);
        g.setGradientFill (juce::ColourGradient::horizontal (shade.withAlpha (leftSide ? 0.9f : 0.0f), toX (leftSide ? 0.0f : span),
                                                             shade.withAlpha (leftSide ? 0.0f : 0.9f), toX (leftSide ? span : 0.0f)));
        g.fillPath (wall);
        return;
    }

    const float span = w * 0.1f;
    const auto outer = palette::melt.darker (0.35f).withAlpha (0.98f);
    const auto inner = palette::violet.interpolatedWith (palette::melt, 0.4f).withAlpha (0.94f);
    g.setGradientFill (juce::ColourGradient::horizontal (leftSide ? outer : inner, leftSide ? 0.0f : w - span,
                                                         leftSide ? inner : outer, leftSide ? span : w));
    g.fillPath (wall);

    // Wax streaks running down the face follow the same flow as the rim.
    for (const float depth : { 0.45f, 0.7f, 0.86f })
    {
        juce::Path streak;
        for (float y = 0.0f; y <= h; y += 8.0f)
        {
            const float x = toX (depth * wallThickness (y, w, h, leftSide, 0));
            if (y < 1.0f)
                streak.startNewSubPath (x, y);
            else
                streak.lineTo (x, y);
        }
        g.setColour (palette::coral.withAlpha (0.035f + 0.03f * depth));
        g.strokePath (streak, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }

    // Wet highlight along the melting face.
    g.setColour (palette::coral.withAlpha (0.2f));
    g.strokePath (rim, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved));
    g.setColour (palette::pearl.withAlpha (0.24f));
    g.strokePath (rim, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
}

void WaterScene::paintCeiling (juce::Graphics& g, juce::Rectangle<float> bounds, int layer) const
{
    const float w = bounds.getWidth();
    const float depth = kCeilingDepth * bounds.getHeight() * (layer == 0 ? 1.0f : 1.6f);

    juce::Path ceiling, rim;
    ceiling.startNewSubPath (0.0f, 0.0f);
    for (float x = 0.0f; x <= w + 4.0f; x += 4.0f)
    {
        // The back layer has no drips of its own, just a slow, deeper undulation.
        const float y = layer == 0 ? ceilingEdge (x, w, depth, 0.0f)
                                   : depth + 12.0f * std::sin (x * 0.006f + static_cast<float> (time) * 0.05f + 2.0f)
                                           + 7.0f * std::sin (x * 0.017f - static_cast<float> (time) * 0.04f);
        ceiling.lineTo (x, y);
        if (x < 1.0f)
            rim.startNewSubPath (x, y);
        else
            rim.lineTo (x, y);
    }
    ceiling.lineTo (w + 4.0f, 0.0f);
    ceiling.closeSubPath();

    if (layer == 1)
    {
        const auto shade = palette::melt.darker (0.8f);
        g.setGradientFill (juce::ColourGradient::vertical (shade.withAlpha (0.9f), 0.0f, shade.withAlpha (0.0f), depth + 20.0f));
        g.fillPath (ceiling);
        return;
    }

    const auto edgeColour = palette::violet.interpolatedWith (palette::melt, 0.4f).withAlpha (0.94f);
    g.setGradientFill (juce::ColourGradient::vertical (palette::melt.darker (0.5f).withAlpha (0.98f), 0.0f, edgeColour, depth + 50.0f));
    g.fillPath (ceiling);

    // Each drip swells a rounded bulb at its tip before it lets go.
    for (const auto& d : drips)
    {
        const float x = d.x * w;
        const float radius = juce::jlimit (3.0f, 9.0f, d.width * w * 0.32f) * (0.55f + 0.45f * smoothstep (d.phase));
        const float tipY = ceilingEdge (x, w, depth, 0.0f);
        const auto bulb = juce::Rectangle<float> (radius * 2.0f, radius * 2.1f).withCentre ({ x, tipY - radius * 0.55f });

        g.setColour (edgeColour);
        g.fillEllipse (bulb);
        g.setColour (palette::coral.withAlpha (0.2f));
        g.drawEllipse (bulb, 1.5f);
        g.setColour (palette::pearl.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (bulb.getCentre().translated (-radius * 0.4f, -radius * 0.2f)));
    }

    g.setColour (palette::coral.withAlpha (0.18f));
    g.strokePath (rim, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved));
    g.setColour (palette::pearl.withAlpha (0.24f));
    g.strokePath (rim, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
}

void WaterScene::paintDroplets (juce::Graphics& g) const
{
    for (const auto& drop : droplets)
    {
        if (! drop.alive)
            continue;

        const float fade = 1.0f - smoothstep (drop.age / 7.0f);
        const float stretch = 1.0f + juce::jmin (drop.velocity / 40.0f, 0.6f);
        const auto r = juce::Rectangle<float> (drop.radius * 2.0f, drop.radius * 2.0f * stretch).withCentre ({ drop.x, drop.y });

        g.setColour (palette::violet.withAlpha (0.55f * fade));
        g.fillEllipse (r);
        g.setColour (palette::coral.withAlpha (0.35f * fade));
        g.drawEllipse (r, 1.0f);
        g.setColour (palette::pearl.withAlpha (0.5f * fade));
        g.fillEllipse (juce::Rectangle<float> (1.6f, 1.6f).withCentre (r.getCentre().translated (-drop.radius * 0.35f, -drop.radius * 0.4f)));
    }
}
} // namespace zither::ui
