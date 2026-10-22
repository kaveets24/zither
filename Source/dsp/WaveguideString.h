#pragma once

#include <array>
#include <vector>

namespace zither
{
struct StringParams
{
    float decaySeconds  = 18.0f; // T60 of the low strings; higher strings ring a little shorter
    float brightness    = 0.6f;  // 0..1, cutoff of the per-period loss filter
    float inharmonicity = 0.2f;  // 0..1, stiffness: stretches upper partials sharp
    float damping       = 0.0f;  // 0..1, how strongly a released (damped) string is muted

    bool operator== (const StringParams&) const = default;
};

/**
    One steel string as a digital waveguide (extended Karplus-Strong).

    Loop: delay line -> Thiran fractional-delay allpass -> dispersion allpass chain
          -> one-pole loss filter -> back into the delay line.

    The phase delay of every filter is measured at the fundamental, and the delay line is
    shortened to compensate. That keeps the string in tune even with heavy dispersion.
*/
class WaveguideString
{
public:
    static constexpr int kDispersionStages = 4;

    void prepare (double sampleRate, double lowestFrequencyHz);
    void reset() noexcept;

    void setFrequency (double hz) noexcept;
    void setParams (const StringParams&) noexcept;
    void setDamped (bool shouldBeDamped) noexcept;

    /** Marks the string as sounding; call when excitation starts. */
    void wake() noexcept;

    float processSample (float excitation) noexcept;

    bool isActive() const noexcept { return active; }
    double getFrequency() const noexcept { return frequency; }
    double getPeriodSamples() const noexcept { return sampleRate / frequency; }
    double getEffectiveT60() const noexcept { return t60; }

private:
    void updateCoefficients() noexcept;

    static constexpr float kSilenceThreshold = 1.0e-5f; // ~ -100 dB
    static constexpr double kDampedT60 = 0.12;

    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
    int delayInt = 1;

    double sampleRate = 44100.0;
    double frequency = 220.0;
    double t60 = 1.0;
    StringParams params;
    bool damped = false;

    float thiranCoef = 0.0f, thiranX1 = 0.0f, thiranY1 = 0.0f;

    float dispCoef = 0.0f;
    std::array<float, kDispersionStages> dispX1 {}, dispY1 {};

    float lossInGain = 0.0f, lossPole = 0.0f, lossState = 0.0f;

    bool active = false;
    float windowPeak = 0.0f;
    int windowCount = 0;
};
} // namespace zither
