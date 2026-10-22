#include "StringField.h"
#include "Palette.h"
#include "../Parameters.h"

#include <cmath>

namespace zither::ui
{
namespace
{
    constexpr float kPi = juce::MathConstants<float>::pi;
    constexpr float kRippleLife = 1.6f;

    const char* const noteNames[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };
} // namespace

StringField::StringField (ZitherAudioProcessor& p)
    : processor (p), parameters (p.getParameters())
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    refreshTuning();

    for (int i = 0; i < kMaxStrings; ++i)
        strings[static_cast<size_t> (i)].seenStrikes = processor.getStrikeCount (i);
}

void StringField::refreshTuning()
{
    auto index = [this] (const char* id) { return static_cast<int> (parameters.getRawParameterValue (id)->load()); };

    const TuningSettings settings { index (param::root), static_cast<Mode> (index (param::mode)),
                                    index (param::numStrings), index (param::lowestOctave) };

    if (settings != tuningSettings || tuning.getNumStrings() == 0)
    {
        tuningSettings = settings;
        tuning.set (settings);
        repaint();
    }
}

juce::Rectangle<float> StringField::stringArea() const
{
    const auto bounds = getLocalBounds().toFloat();
    return bounds.reduced (juce::jmax (72.0f, bounds.getWidth() * 0.11f), 22.0f);
}

float StringField::stringSpacing() const
{
    const int n = tuning.getNumStrings();
    return n > 1 ? stringArea().getHeight() / static_cast<float> (n - 1) : 0.0f;
}

float StringField::stringY (int index) const
{
    // Low strings nearest the player (bottom), like a zither on your lap.
    return stringArea().getBottom() - static_cast<float> (index) * stringSpacing();
}

int StringField::stringNear (float y, float tolerance) const
{
    const float spacing = stringSpacing();
    if (spacing <= 0.0f)
        return tuning.getNumStrings() == 1 ? 0 : -1;

    const int index = juce::roundToInt ((stringArea().getBottom() - y) / spacing);
    if (index < 0 || index >= tuning.getNumStrings() || std::abs (stringY (index) - y) > tolerance)
        return -1;

    return index;
}

void StringField::update (double deltaSeconds, bool animate)
{
    refreshTuning();

    const auto dt = static_cast<float> (deltaSeconds);
    const int n = tuning.getNumStrings();
    const auto strikeX = stringArea().getX() + stringArea().getWidth() * parameters.getRawParameterValue (param::strikePos)->load();
    const double now = juce::Time::getMillisecondCounterHiRes();

    for (int i = 0; i < kMaxStrings; ++i)
    {
        auto& s = strings[static_cast<size_t> (i)];
        const float peak = processor.takeStringLevel (i);
        const uint32_t strikes = processor.getStrikeCount (i);
        const bool struck = strikes != s.seenStrikes;
        s.seenStrikes = strikes;

        if (! animate || i >= n)
        {
            s.level = 0.0f;
            continue;
        }

        // Glow follows loudness in dB, so a long ring fades gradually instead of staying lit until the end.
        const float target = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (peak, -100.0f) + 48.0f) / 40.0f);
        s.level = target > s.level ? target : s.level + (target - s.level) * juce::jmin (1.0f, dt * 4.0f);

        const float rate = 10.0f + 14.0f * (n > 1 ? static_cast<float> (i) / static_cast<float> (n - 1) : 0.0f);
        s.phase = std::fmod (s.phase + dt * rate, 2.0f * kPi * 100.0f);

        if (struck)
        {
            // Hands never land in exactly the same spot: scatter the strike point a little per string.
            const bool touched = s.touchX >= 0.0f && now - s.touchTime < 250.0;
            const float scatter = stringArea().getWidth() * 0.07f * std::sin (static_cast<float> (i) * 2.39f);
            spawnEffects (i, touched ? s.touchX : strikeX + scatter, juce::jmax (0.35f, target));
            s.touchX = -1.0f;
        }
    }

    if (! animate)
    {
        for (auto& r : ripples)
            r.alive = false;
        for (auto& b : bubbles)
            b.alive = false;

        if (std::exchange (animating, false))
            repaint();
        return;
    }

    animating = true;

    for (auto& r : ripples)
        if (r.alive && (r.age += dt) > kRippleLife)
            r.alive = false;

    for (auto& b : bubbles)
    {
        if (! b.alive)
            continue;

        b.age += dt;
        b.y -= b.rise * dt;
        b.x += std::sin (b.phase + b.age * 1.7f) * b.sway * dt;
        b.alive = b.age < b.life && b.y > -10.0f;
    }

    // A few ambient bubbles drift up even in silence, so the water always feels alive.
    ambientTimer -= dt;
    if (ambientTimer <= 0.0f)
    {
        ambientTimer = 0.6f + 1.2f * random.nextFloat();
        const auto area = stringArea();
        spawnBubble (area.getX() + random.nextFloat() * area.getWidth(), static_cast<float> (getHeight()) + 4.0f,
                     1.0f + 2.0f * random.nextFloat(), 10.0f + 14.0f * random.nextFloat());
    }

    clock += deltaSeconds;
    repaint();
}

void StringField::spawnEffects (int index, float x, float strength)
{
    const float y = stringY (index);

    for (auto& r : ripples)
    {
        if (r.alive)
            continue;

        r = { x, y, 0.0f, strength, true };
        break;
    }

    const int count = 2 + juce::roundToInt (strength * 4.0f);
    for (int i = 0; i < count; ++i)
        spawnBubble (x + (random.nextFloat() - 0.5f) * 24.0f, y - 2.0f,
                     0.8f + 2.6f * random.nextFloat() * strength, 18.0f + 26.0f * random.nextFloat());
}

void StringField::spawnBubble (float x, float y, float radius, float rise)
{
    for (auto& b : bubbles)
    {
        if (b.alive)
            continue;

        b = { x, y, radius, rise, 4.0f + 8.0f * random.nextFloat(), random.nextFloat() * 2.0f * kPi,
              0.0f, 3.5f + 3.0f * random.nextFloat(), true };
        return;
    }
}

void StringField::strike (int index, float x, float velocity)
{
    auto& s = strings[static_cast<size_t> (index)];
    s.touchX = x;
    s.touchTime = juce::Time::getMillisecondCounterHiRes();
    processor.strikeString (index, velocity);
}

void StringField::mouseDown (const juce::MouseEvent& e)
{
    lastDrag = e.position;
    lastDragTime = juce::Time::getMillisecondCounterHiRes();

    if (const int index = stringNear (e.position.y, juce::jmax (4.0f, stringSpacing() * 0.5f)); index >= 0)
        strike (index, e.position.x, 0.7f);
}

void StringField::mouseDrag (const juce::MouseEvent& e)
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float fromY = lastDrag.y;
    const float toY = e.position.y;
    const float speed = std::abs (toY - fromY) / static_cast<float> (juce::jmax (1.0, now - lastDragTime)); // px per ms
    const float velocity = juce::jlimit (0.25f, 1.0f, 0.3f + speed * 0.35f);

    // Strike every string the pointer crossed since the last event, in the order it crossed them.
    const int n = tuning.getNumStrings();
    const bool movingUp = toY < fromY;
    for (int k = 0; k < n; ++k)
    {
        const int i = movingUp ? k : n - 1 - k;
        const float y = stringY (i);
        const bool crossed = movingUp ? (y < fromY && y >= toY) : (y > fromY && y <= toY);
        if (crossed)
            strike (i, e.position.x, velocity);
    }

    lastDrag = e.position;
    lastDragTime = now;
}

void StringField::paint (juce::Graphics& g)
{
    const auto area = stringArea();
    const int n = tuning.getNumStrings();
    const float spacing = stringSpacing();
    const int rootPitchClass = ((tuningSettings.rootPitchClass % 12) + 12) % 12;

    for (int i = 0; i < n; ++i)
    {
        const auto& s = strings[static_cast<size_t> (i)];
        const float y = stringY (i);
        const float shade = n > 1 ? static_cast<float> (i) / static_cast<float> (n - 1) : 0.0f;
        const float thickness = 1.7f - 0.9f * shade;
        const bool isRoot = tuning.getStringNote (i) % 12 == rootPitchClass;
        const auto base = isRoot ? palette::coral : palette::pearl;
        const float level = animating ? s.level : 0.0f;

        juce::Path line;
        if (level > 0.002f)
        {
            // A few standing-wave modes, pinned at both bridges.
            const float amp = level * juce::jmin (spacing * 0.42f, 14.0f);
            constexpr int segments = 48;
            for (int k = 0; k <= segments; ++k)
            {
                const float u = static_cast<float> (k) / segments;
                const float disp = amp * (std::sin (kPi * u) * std::sin (s.phase)
                                          + 0.35f * std::sin (2.0f * kPi * u) * std::sin (1.9f * s.phase + 0.7f)
                                          + 0.15f * std::sin (3.0f * kPi * u) * std::sin (3.1f * s.phase + 1.3f));
                const float x = area.getX() + u * area.getWidth();
                if (k == 0)
                    line.startNewSubPath (x, y + disp);
                else
                    line.lineTo (x, y + disp);
            }

            g.setColour (palette::aqua.withAlpha (0.2f * level));
            g.strokePath (line, juce::PathStrokeType (thickness + 6.0f * level, juce::PathStrokeType::curved));
        }
        else
        {
            line.startNewSubPath (area.getX(), y);
            line.lineTo (area.getRight(), y);
        }

        const float restAlpha = isRoot ? 0.32f : 0.17f;
        g.setColour (base.interpolatedWith (palette::aqua, level * 0.6f).withAlpha (restAlpha + (1.0f - restAlpha) * level));
        g.strokePath (line, juce::PathStrokeType (thickness));

        // Bridge pins.
        g.setColour (palette::pearl.withAlpha (0.22f));
        g.fillEllipse (juce::Rectangle<float> (3.5f, 3.5f).withCentre ({ area.getX() - 6.0f, y }));
        g.fillEllipse (juce::Rectangle<float> (3.5f, 3.5f).withCentre ({ area.getRight() + 6.0f, y }));

        if (isRoot)
        {
            g.setColour (palette::coral.withAlpha (0.5f));
            g.setFont (palette::font (10.0f, 0.1f));
            g.drawText (noteNames[rootPitchClass], juce::Rectangle<float> (24.0f, 12.0f).withCentre ({ area.getX() - 22.0f, y }),
                        juce::Justification::centredRight);
        }
    }

    if (! animating)
        return;

    for (const auto& r : ripples)
    {
        if (! r.alive)
            continue;

        const float t = r.age / kRippleLife;
        const float radius = 5.0f + t * 48.0f;
        const float fade = (1.0f - t) * (1.0f - t);
        g.setColour (palette::aqua.withAlpha (0.45f * r.strength * fade));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 0.8f).withCentre ({ r.x, r.y }), 1.0f);
        g.setColour (palette::pearl.withAlpha (0.2f * r.strength * fade));
        g.drawEllipse (juce::Rectangle<float> (radius * 1.2f, radius * 0.48f).withCentre ({ r.x, r.y }), 0.8f);
    }

    for (const auto& b : bubbles)
    {
        if (! b.alive)
            continue;

        const float fadeIn = juce::jmin (1.0f, b.age * 3.0f);
        const float fadeOut = juce::jmin (1.0f, (b.life - b.age) * 0.8f);
        const float alpha = fadeIn * fadeOut;
        const auto r = juce::Rectangle<float> (b.radius * 2.0f, b.radius * 2.0f).withCentre ({ b.x, b.y });

        g.setColour (palette::pearl.withAlpha (0.35f * alpha));
        g.drawEllipse (r, 0.9f);
        g.setColour (palette::pearl.withAlpha (0.6f * alpha));
        g.fillEllipse (juce::Rectangle<float> (b.radius * 0.6f, b.radius * 0.6f).withCentre (r.getCentre().translated (-b.radius * 0.35f, -b.radius * 0.35f)));
    }
}
} // namespace zither::ui
