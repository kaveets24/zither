#pragma once

#include "../dsp/Random.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace zither
{
enum class StrumDirection : int
{
    Up = 0,
    Down,
    Alternate
};

struct StrumSettings
{
    bool enabled = true;
    float intervalMs = 40.0f; // time between neighbouring strings
    int span = 4;             // strings per strum, including the played one
    StrumDirection direction = StrumDirection::Up;
    float humanize = 0.35f;   // 0..1, timing and velocity looseness
};

/**
    Turns one key press into a sweep across neighbouring strings. Strikes are scheduled on a
    sample-accurate timeline, so a strum can cross block boundaries.
*/
class Strummer
{
public:
    void prepare (double newSampleRate) noexcept;
    void setSettings (const StrumSettings& newSettings) noexcept { settings = newSettings; }
    void clear() noexcept { numPending = 0; }

    /** Strikes the played string immediately and queues the rest of the strum. */
    template <typename StrikeFn>
    void noteOn (int stringIndex, float velocity, int numStrings, StrikeFn&& strike);

    /** Samples until the next queued strike, or -1 if nothing is pending. */
    int samplesUntilNext() const noexcept;

    void advance (int numSamples) noexcept { now += numSamples; }

    /** Fires every strike that is due at the current time. */
    template <typename StrikeFn>
    void popDue (StrikeFn&& strike);

    int getNumPending() const noexcept { return numPending; }

private:
    struct Strike
    {
        int64_t time;
        int stringIndex;
        float velocity;
    };

    void schedule (int64_t time, int stringIndex, float velocity) noexcept;

    static constexpr int kCapacity = 256;
    std::array<Strike, kCapacity> pending {};
    int numPending = 0;
    int64_t now = 0;
    double sampleRate = 44100.0;
    StrumSettings settings;
    bool nextAlternateUp = true;
    uint32_t rng = 0x2545F491u;
};

template <typename StrikeFn>
void Strummer::noteOn (int stringIndex, float velocity, int numStrings, StrikeFn&& strike)
{
    if (stringIndex < 0 || stringIndex >= numStrings)
        return;

    strike (stringIndex, velocity);

    if (! settings.enabled || settings.span <= 1)
        return;

    bool up = settings.direction == StrumDirection::Up;
    if (settings.direction == StrumDirection::Alternate)
    {
        up = nextAlternateUp;
        nextAlternateUp = ! nextAlternateUp;
    }

    const double interval = settings.intervalMs * 0.001 * sampleRate;

    for (int i = 1; i < settings.span; ++i)
    {
        const int index = stringIndex + (up ? i : -i);
        if (index < 0 || index >= numStrings)
            break;

        const double position = i + settings.humanize * 0.35 * nextBipolarRandom (rng);
        const auto offset = std::max<int64_t> (1, std::llround (position * interval));
        const auto v = velocity * std::pow (0.86f, static_cast<float> (i)) * (1.0f + settings.humanize * 0.2f * nextBipolarRandom (rng));
        schedule (now + offset, index, std::clamp (v, 0.02f, 1.0f));
    }
}

template <typename StrikeFn>
void Strummer::popDue (StrikeFn&& strike)
{
    for (int i = 0; i < numPending;)
    {
        if (pending[static_cast<size_t> (i)].time <= now)
        {
            const auto due = pending[static_cast<size_t> (i)];
            pending[static_cast<size_t> (i)] = pending[static_cast<size_t> (--numPending)];
            strike (due.stringIndex, due.velocity);
        }
        else
        {
            ++i;
        }
    }
}
} // namespace zither
