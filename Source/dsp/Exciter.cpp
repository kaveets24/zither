#include "Exciter.h"
#include "Random.h"

#include <algorithm>
#include <cmath>

namespace zither
{
namespace
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kSoftestHammerMs = 4.0;
    constexpr double kHardestHammerMs = 0.35;
} // namespace

void Exciter::prepare (double newSampleRate, int maxPeriodSamples)
{
    sampleRate = newSampleRate;
    maxComb = maxPeriodSamples;
    maxRaw = std::max (maxPeriodSamples, static_cast<int> (std::ceil (kSoftestHammerMs * 0.001 * sampleRate))) + 8;
    raw.assign (static_cast<size_t> (maxRaw), 0.0f);
    burst.assign (static_cast<size_t> (maxRaw + maxComb), 0.0f);
    reset();
}

void Exciter::trigger (const ExciterSettings& settings, float velocity, double periodSamples, uint32_t& rngState) noexcept
{
    if (raw.empty())
        return;

    const double vel = std::clamp (static_cast<double> (velocity), 0.0, 1.0);
    const double bright = std::clamp (0.2 + 0.5 * settings.hardness + 0.3 * vel, 0.0, 1.0);
    double amp = 0.05 + 0.95 * std::pow (vel, 1.5);
    int rawLength = 0;

    if (settings.type == ExciterType::Hammer)
    {
        // Harder hammers and faster hits have shorter contact times, so the strike is brighter.
        // Short treble strings push the hammer back sooner, so cap contact at about half a period.
        const double maxWidthMs = 1000.0 * 0.6 * periodSamples / sampleRate;
        const double widthMs = std::min (maxWidthMs, kSoftestHammerMs * std::pow (kHardestHammerMs / kSoftestHammerMs, bright));
        rawLength = std::clamp (static_cast<int> (std::lround (widthMs * 0.001 * sampleRate)), 2, maxRaw);

        for (int n = 0; n < rawLength; ++n)
            raw[static_cast<size_t> (n)] = static_cast<float> (0.5 * (1.0 - std::cos (2.0 * kPi * (n + 0.5) / rawLength)));

        // Narrow pulses carry less low-end energy; compensate partially so hard hits aren't quieter.
        amp *= std::pow (kSoftestHammerMs / widthMs, 0.35);
    }
    else
    {
        rawLength = std::clamp (static_cast<int> (std::lround (periodSamples)), 4, maxRaw);
        const double q = 0.92 * (1.0 - bright);
        double lp = 0.0, mean = 0.0;

        for (int n = 0; n < rawLength; ++n)
        {
            lp = (1.0 - q) * nextBipolarRandom (rngState) + q * lp;
            raw[static_cast<size_t> (n)] = static_cast<float> (lp);
            mean += lp;
        }

        mean /= rawLength;
        float peak = 1.0e-9f;
        for (int n = 0; n < rawLength; ++n)
        {
            auto& s = raw[static_cast<size_t> (n)];
            s -= static_cast<float> (mean);
            peak = std::max (peak, std::abs (s));
        }

        for (int n = 0; n < rawLength; ++n)
            raw[static_cast<size_t> (n)] /= peak;
    }

    const int combDelay = std::clamp (static_cast<int> (std::lround (settings.strikePosition * periodSamples)), 1, maxComb);
    length = rawLength + combDelay;

    const auto scale = static_cast<float> (0.5 * amp);
    for (int n = 0; n < length; ++n)
    {
        const float direct = n < rawLength ? raw[static_cast<size_t> (n)] : 0.0f;
        const float reflected = (n >= combDelay && n - combDelay < rawLength) ? raw[static_cast<size_t> (n - combDelay)] : 0.0f;
        burst[static_cast<size_t> (n)] = scale * (direct - reflected);
    }

    readPos = 0;
}
} // namespace zither
