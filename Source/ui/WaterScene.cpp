#include "WaterScene.h"
#include "Palette.h"

#include <array>
#include <cmath>
#include <utility>

namespace zither::ui
{
namespace
{
    // The underside of the surface, hanging from the top edge, deepest first. Neighbouring layers travel in
    // opposite directions, so where they cross the light gathers and parts the way it does under real chop.
    // Offsets are in px because the header above the strings is a fixed height.
    constexpr std::array<WaterScene::Wave, 4> surfaceWaves { {
        { 68.0f, 11.0f, 0.0058f, 0.22f, 0.0f },
        { 52.0f, 9.0f, 0.0084f, -0.30f, 2.1f },
        { 36.0f, 7.0f, 0.0118f, 0.38f, 4.0f },
        { 20.0f, 5.0f, 0.0160f, -0.46f, 5.6f },
    } };

    // Swells rolling along the bottom, measured up from the bottom edge, farthest first. Nearer swells are taller,
    // quicker and darker. They stay below the strings and behind the fixed-height control panel at any size.
    constexpr std::array<WaterScene::Wave, 4> swellWaves { {
        { 225.0f, 16.0f, 0.0040f, 0.08f, 0.6f },
        { 175.0f, 20.0f, 0.0055f, -0.11f, 3.3f },
        { 118.0f, 24.0f, 0.0072f, 0.14f, 5.1f },
        { 58.0f, 28.0f, 0.0094f, -0.17f, 1.4f },
    } };

    constexpr float kStep = 6.0f; // px between wave samples; the paths are stroked as curves anyway
} // namespace

WaterScene::WaterScene()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, false);
}

void WaterScene::advance (double deltaSeconds)
{
    time += deltaSeconds;
}

float WaterScene::waveY (const Wave& wave, float x, float baseline) const noexcept
{
    const auto t = static_cast<float> (time);
    return baseline
         + wave.amplitude * std::sin (wave.frequency * x + wave.speed * t + wave.phase)
         + 0.4f * wave.amplitude * std::sin (2.31f * wave.frequency * x - 0.7f * wave.speed * t + 1.9f * wave.phase);
}

juce::Path WaterScene::waveLine (const Wave& wave, float width, float baseline) const
{
    juce::Path line;
    line.startNewSubPath (-kStep, waveY (wave, -kStep, baseline));
    for (float x = 0.0f; x <= width + kStep; x += kStep)
        line.lineTo (x, waveY (wave, x, baseline));
    return line;
}

juce::Path WaterScene::closedTo (juce::Path line, float width, float y)
{
    line.lineTo (width + kStep, y);
    line.lineTo (-kStep, y);
    line.closeSubPath();
    return line;
}

void WaterScene::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    paintWater (g, bounds);
    paintLight (g, bounds);
    paintSwells (g, bounds);
    paintSurface (g, bounds);
    paintVignette (g, bounds);
}

void WaterScene::paintWater (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    // Brighter near the surface, sinking to the abyss.
    juce::ColourGradient water (palette::lagoon, 0.0f, bounds.getY(), palette::abyss, 0.0f, bounds.getBottom(), false);
    water.addColour (0.35, palette::deepWater);
    g.setGradientFill (water);
    g.fillRect (bounds);

    // A soft glow in the middle of the water, where the strings hang.
    g.setGradientFill (juce::ColourGradient (palette::lagoon.withAlpha (0.4f), bounds.getCentreX(), bounds.getHeight() * 0.45f,
                                             juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getBottom(), true));
    g.fillRect (bounds);
}

void WaterScene::paintSurface (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float alphas[] = { 0.05f, 0.07f, 0.09f, 0.12f };

    for (size_t i = 0; i < surfaceWaves.size(); ++i)
    {
        const auto& wave = surfaceWaves[i];
        const auto rim = waveLine (wave, bounds.getWidth(), wave.offset);
        const auto sheet = closedTo (rim, bounds.getWidth(), -kStep);
        const float bottom = wave.offset + wave.amplitude * 1.4f;

        g.setGradientFill (juce::ColourGradient::vertical (palette::aqua.withAlpha (alphas[i] * 1.6f), 0.0f,
                                                           palette::aqua.withAlpha (alphas[i] * 0.5f), bottom));
        g.fillPath (sheet);

        // The line where the surface turns light back down: a soft glow with a bright thread through it.
        g.setColour (palette::aqua.withAlpha (alphas[i] * 0.8f));
        g.strokePath (rim, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved));
        g.setColour (palette::pearl.withAlpha (alphas[i] * 3.2f));
        g.strokePath (rim, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }
}

void WaterScene::paintLight (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const auto t = static_cast<float> (time);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Shafts of light falling from the surface, swaying slowly.
    for (int i = 0; i < 4; ++i)
    {
        const float fi = static_cast<float> (i);
        const float top = w * (0.2f + 0.2f * fi) + 30.0f * std::sin (t * 0.05f + fi * 2.1f);
        const float spread = w * (0.03f + 0.01f * fi);
        const float lean = w * 0.08f * std::sin (t * 0.03f + fi);

        juce::Path ray;
        ray.startNewSubPath (top - spread * 0.5f, 0.0f);
        ray.lineTo (top + spread * 0.5f, 0.0f);
        ray.lineTo (top + lean + spread * 1.8f, h * 0.8f);
        ray.lineTo (top + lean - spread * 1.8f, h * 0.8f);
        ray.closeSubPath();

        const float shimmer = 0.06f + 0.025f * std::sin (t * 0.21f + fi * 1.3f);
        g.setGradientFill (juce::ColourGradient::vertical (palette::aqua.withAlpha (shimmer), 0.0f,
                                                           juce::Colours::transparentBlack, h * 0.8f));
        g.fillPath (ray);
    }

    // Caustic bands: light refracted by the surface waves, drifting through the middle water.
    for (int i = 0; i < 4; ++i)
    {
        const float fi = static_cast<float> (i);
        const float baseY = h * (0.2f + 0.14f * fi);

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

        g.setColour (palette::aqua.withAlpha (0.02f));
        g.strokePath (band, juce::PathStrokeType (26.0f, juce::PathStrokeType::curved));
        g.setColour (palette::pearl.withAlpha (0.022f));
        g.strokePath (band, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
    }
}

void WaterScene::paintSwells (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float h = bounds.getHeight();
    const float bottom = h + kStep;

    // Far swells dissolve into the water colour; near ones are darker and catch more light on their crests.
    const juce::Colour tints[] = { palette::lagoon.brighter (0.15f).withAlpha (0.45f),
                                   palette::lagoon.withAlpha (0.6f),
                                   palette::lagoon.interpolatedWith (palette::deepWater, 0.6f).withAlpha (0.75f),
                                   palette::deepWater.interpolatedWith (palette::abyss, 0.5f).withAlpha (0.9f) };
    const float crestAlphas[] = { 0.12f, 0.16f, 0.2f, 0.24f };

    for (size_t i = 0; i < swellWaves.size(); ++i)
    {
        const auto& wave = swellWaves[i];
        const float baseline = h - wave.offset;
        const auto rim = waveLine (wave, bounds.getWidth(), baseline);
        const auto body = closedTo (rim, bounds.getWidth(), bottom);
        const float crest = baseline - wave.amplitude * 1.4f;

        g.setGradientFill (juce::ColourGradient::vertical (tints[i], crest, palette::abyss.withAlpha (tints[i].getFloatAlpha()), bottom));
        g.fillPath (body);

        g.setColour (palette::aqua.withAlpha (crestAlphas[i] * 0.35f));
        g.strokePath (rim, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
        g.setColour (palette::aqua.withAlpha (crestAlphas[i]));
        g.strokePath (rim, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
    }
}

void WaterScene::paintVignette (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    // The edges of the view fall away into deep water rather than ending at a frame.
    const auto centre = bounds.getCentre();
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, centre.x, centre.y,
                                             palette::abyss.withAlpha (0.55f), 0.0f, 0.0f, true));
    g.fillRect (bounds);
}
} // namespace zither::ui
