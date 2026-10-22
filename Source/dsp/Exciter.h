#pragma once

#include <cstdint>
#include <vector>

namespace zither
{
enum class ExciterType : int
{
    Hammer = 0, // felt mallet: raised-cosine force pulse
    Pluck       // pick/finger: filtered noise burst
};

struct ExciterSettings
{
    ExciterType type = ExciterType::Hammer;
    float hardness = 0.35f;       // 0..1, softer = darker and rounder
    float strikePosition = 0.13f; // fraction of string length from the bridge
};

/**
    Builds a short excitation signal for one string and plays it into the waveguide.

    A comb at the strike position (e[n] - e[n - b]) gives each strike its timbre. Because the comb
    removes DC, no offset gets trapped in the high-feedback string loop.
*/
class Exciter
{
public:
    void prepare (double sampleRate, int maxPeriodSamples);
    void reset() noexcept { length = readPos = 0; }

    void trigger (const ExciterSettings&, float velocity, double periodSamples, uint32_t& rngState) noexcept;

    bool isActive() const noexcept { return readPos < length; }
    float nextSample() noexcept { return readPos < length ? burst[static_cast<size_t> (readPos++)] : 0.0f; }

private:
    double sampleRate = 44100.0;
    int maxRaw = 0;
    int maxComb = 0;
    std::vector<float> raw, burst;
    int length = 0;
    int readPos = 0;
};
} // namespace zither
