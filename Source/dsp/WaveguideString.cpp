#include "WaveguideString.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace zither
{
namespace
{
    constexpr double kPi = 3.14159265358979323846;
    using Complex = std::complex<double>;

    // H(z) = (a + z^-1) / (1 + a z^-1)
    double allpassPhaseDelay (double a, double w)
    {
        const auto z1 = std::polar (1.0, -w);
        return -std::arg ((a + z1) / (1.0 + a * z1)) / w;
    }

    // H(z) = (1 - p) / (1 - p z^-1), unity gain at DC
    Complex onePoleResponse (double p, double w)
    {
        return (1.0 - p) / (1.0 - p * std::polar (1.0, -w));
    }
} // namespace

void WaveguideString::prepare (double newSampleRate, double lowestFrequencyHz)
{
    sampleRate = newSampleRate;

    const auto maxDelay = static_cast<int> (std::ceil (sampleRate / lowestFrequencyHz)) + 8;
    int size = 1;
    while (size < maxDelay)
        size <<= 1;

    buffer.assign (static_cast<size_t> (size), 0.0f);
    mask = size - 1;

    reset();
    updateCoefficients();
}

void WaveguideString::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
    thiranX1 = thiranY1 = 0.0f;
    dispX1.fill (0.0f);
    dispY1.fill (0.0f);
    lossState = 0.0f;
    active = false;
    windowPeak = 0.0f;
    windowCount = 0;
}

void WaveguideString::setFrequency (double hz) noexcept
{
    frequency = std::clamp (hz, 20.0, sampleRate * 0.45);
    updateCoefficients();
}

void WaveguideString::setParams (const StringParams& newParams) noexcept
{
    if (newParams == params)
        return;

    params = newParams;
    updateCoefficients();
}

void WaveguideString::setDamped (bool shouldBeDamped) noexcept
{
    if (shouldBeDamped == damped)
        return;

    damped = shouldBeDamped;
    updateCoefficients();
}

void WaveguideString::wake() noexcept
{
    active = true;
    windowPeak = 0.0f;
    windowCount = 0;
}

void WaveguideString::updateCoefficients() noexcept
{
    if (buffer.empty())
        return;

    const double period = sampleRate / frequency;
    const double w0 = 2.0 * kPi * frequency / sampleRate;

    // Decay: low strings ring longest, as on a real zither.
    const double baseT60 = std::max (0.05, params.decaySeconds * std::clamp (std::pow (130.81 / frequency, 0.4), 0.25, 1.5));
    t60 = baseT60;
    if (damped && params.damping > 0.0f && baseT60 > kDampedT60)
        t60 = std::exp (std::log (baseT60) + (std::log (kDampedT60) - std::log (baseT60)) * params.damping);

    // Loss filter from two decay targets: t60 at the fundamental and a shorter t60 at a high reference
    // frequency (brightness sets the ratio). Specifying it in Hz keeps short treble strings alive and
    // the timbre independent of sample rate.
    const double gTarget = std::pow (10.0, -3.0 / (frequency * t60));
    const double highRatio = 0.03 * std::pow (0.8 / 0.03, std::clamp (static_cast<double> (params.brightness), 0.0, 1.0));
    const double gHigh = std::pow (10.0, -3.0 / (frequency * t60 * highRatio));
    const double wHigh = 2.0 * kPi * std::min (std::max (6000.0, 1.5 * frequency), 0.45 * sampleRate) / sampleRate;

    // Solve |(1 - p) / (1 - p e^-jw)|^2 = (gHigh / gTarget)^2 for the pole p in [0, 1).
    const double r = std::pow (gHigh / gTarget, 2.0);
    const double qa = 1.0 - r;
    const double qb = 2.0 - 2.0 * r * std::cos (wHigh);
    const double p = qa > 1.0e-12 ? std::clamp ((qb - std::sqrt (std::max (0.0, qb * qb - 4.0 * qa * qa))) / (2.0 * qa), 0.0, 0.95) : 0.0;
    const auto loss = onePoleResponse (p, w0);

    // Per-period gain for the requested T60, corrected for the filter's own attenuation at f0.
    const double g = std::min (0.99999, gTarget / std::abs (loss));
    const double tauLoss = -std::arg (loss) / w0;

    // Dispersion: negative allpass coefficients delay lows more than highs -> stretched partials.
    // Back off the amount for very short (high) strings so the delay line keeps a sane length.
    const double budget = 0.5 * (period - tauLoss - 1.6);
    double a = -0.7 * std::clamp (static_cast<double> (params.inharmonicity), 0.0, 1.0);
    double tauDisp = kDispersionStages * allpassPhaseDelay (a, w0);
    while (a < -1.0e-4 && tauDisp > budget)
    {
        a *= 0.7;
        tauDisp = kDispersionStages * allpassPhaseDelay (a, w0);
    }

    // What's left goes to the integer delay plus a Thiran allpass for the fraction (d in [0.5, 1.5)).
    const double remaining = period - tauLoss - tauDisp;
    const int length = std::clamp (static_cast<int> (std::floor (remaining - 0.5)), 1, mask - 1);
    const double target = remaining - length;

    // The Thiran delay is exact only at DC; refine it so it's exact at the fundamental.
    double d = target;
    for (int i = 0; i < 4; ++i)
    {
        const double eta = (1.0 - d) / (1.0 + d);
        d = std::clamp (d + target - allpassPhaseDelay (eta, w0), 0.05, 3.0);
    }

    delayInt = length;
    thiranCoef = static_cast<float> ((1.0 - d) / (1.0 + d));
    dispCoef = static_cast<float> (a);
    lossPole = static_cast<float> (p);
    lossInGain = static_cast<float> (g * (1.0 - p));
}

float WaveguideString::processSample (float excitation) noexcept
{
    const float x = buffer[static_cast<size_t> ((writePos - delayInt) & mask)];

    float y = thiranCoef * x + thiranX1 - thiranCoef * thiranY1;
    thiranX1 = x;
    thiranY1 = y;

    for (size_t k = 0; k < kDispersionStages; ++k)
    {
        const float o = dispCoef * y + dispX1[k] - dispCoef * dispY1[k];
        dispX1[k] = y;
        dispY1[k] = o;
        y = o;
    }

    lossState = lossInGain * y + lossPole * lossState;

    const float out = lossState + excitation;
    buffer[static_cast<size_t> (writePos)] = out;
    writePos = (writePos + 1) & mask;

    // Go to sleep once a full period (plus margin) has stayed below the silence threshold.
    windowPeak = std::max (windowPeak, std::abs (out));
    if (++windowCount >= delayInt + 64)
    {
        if (windowPeak < kSilenceThreshold)
            reset();

        windowPeak = 0.0f;
        windowCount = 0;
    }

    return out;
}
} // namespace zither
