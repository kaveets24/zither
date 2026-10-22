#include "StringBank.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace zither
{
void StringBank::prepare (double sampleRate)
{
    const auto maxPeriod = static_cast<int> (std::ceil (sampleRate / kLowestFrequencyHz)) + 8;

    for (auto& s : strings)
        s.prepare (sampleRate, kLowestFrequencyHz);

    for (auto& e : exciters)
        e.prepare (sampleRate, maxPeriod);

    dcCoef = static_cast<float> (std::exp (-2.0 * 3.14159265358979323846 * 10.0 / sampleRate));
    reset();
}

void StringBank::reset() noexcept
{
    for (auto& s : strings)
        s.reset();

    for (auto& e : exciters)
        e.reset();

    dcX1.fill (0.0f);
    dcY1.fill (0.0f);
}

void StringBank::setTuning (const Tuning& tuning) noexcept
{
    numStrings = tuning.getNumStrings();

    for (int i = 0; i < kMaxStrings; ++i)
    {
        const auto idx = static_cast<size_t> (i);

        if (i >= numStrings)
        {
            strings[idx].reset();
            exciters[idx].reset();
            continue;
        }

        strings[idx].setFrequency (tuning.getStringFrequency (i));

        // Gentle spread: bass strings slightly left, treble slightly right, as if seated at the instrument.
        const double position = numStrings > 1 ? static_cast<double> (i) / (numStrings - 1) : 0.5;
        const double angle = (0.1 + 0.8 * position) * 0.5 * 3.14159265358979323846;
        gainLeft[idx] = static_cast<float> (0.5 * std::cos (angle));
        gainRight[idx] = static_cast<float> (0.5 * std::sin (angle));
    }
}

void StringBank::setStringParams (const StringParams& params) noexcept
{
    for (auto& s : strings)
        s.setParams (params);
}

void StringBank::excite (int stringIndex, float velocity) noexcept
{
    if (stringIndex < 0 || stringIndex >= numStrings)
        return;

    auto& s = strings[static_cast<size_t> (stringIndex)];
    exciters[static_cast<size_t> (stringIndex)].trigger (exciterSettings, velocity, s.getPeriodSamples(), rng);
    s.setDamped (false);
    s.wake();
    ++strikeCounts[static_cast<size_t> (stringIndex)];
}

float StringBank::takePeak (int stringIndex) noexcept
{
    return std::exchange (peaks[static_cast<size_t> (stringIndex)], 0.0f);
}

void StringBank::setDamped (int stringIndex, bool shouldBeDamped) noexcept
{
    if (stringIndex >= 0 && stringIndex < numStrings)
        strings[static_cast<size_t> (stringIndex)].setDamped (shouldBeDamped);
}

void StringBank::render (float* left, float* right, int numSamples) noexcept
{
    for (size_t i = 0; i < static_cast<size_t> (numStrings); ++i)
    {
        auto& s = strings[i];
        if (! s.isActive())
            continue;

        auto& e = exciters[i];
        const float gl = gainLeft[i];
        const float gr = gainRight[i];
        float peak = peaks[i];

        for (int n = 0; n < numSamples; ++n)
        {
            const float v = s.processSample (e.nextSample());
            left[n] += v * gl;
            right[n] += v * gr;
            peak = std::max (peak, std::abs (v));
        }

        peaks[i] = peak;
    }

    // DC blocker on the summed output (the string loops pass DC almost unattenuated).
    float* channels[] = { left, right };
    for (size_t ch = 0; ch < 2; ++ch)
    {
        float* data = channels[ch];
        for (int n = 0; n < numSamples; ++n)
        {
            const float x = data[n];
            const float y = x - dcX1[ch] + dcCoef * dcY1[ch];
            dcX1[ch] = x;
            dcY1[ch] = y;
            data[n] = y;
        }
    }
}
} // namespace zither
