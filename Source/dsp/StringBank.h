#pragma once

#include "Exciter.h"
#include "WaveguideString.h"
#include "../tuning/Tuning.h"

#include <array>
#include <cstdint>

namespace zither
{
/**
    The whole instrument body: a fixed set of always-available strings. There is no voice
    stealing. A string is either ringing or asleep, and it sleeps on its own once it falls silent.
*/
class StringBank
{
public:
    static constexpr int kMaxStrings = Tuning::kMaxStrings;
    static constexpr double kLowestFrequencyHz = 30.0;

    void prepare (double sampleRate);
    void reset() noexcept;

    void setTuning (const Tuning&) noexcept;
    void setStringParams (const StringParams&) noexcept;
    void setExciterSettings (const ExciterSettings& settings) noexcept { exciterSettings = settings; }

    void excite (int stringIndex, float velocity) noexcept;
    void setDamped (int stringIndex, bool shouldBeDamped) noexcept;

    /** Adds the strings' stereo output into the buffers. */
    void render (float* left, float* right, int numSamples) noexcept;

    int getNumStrings() const noexcept { return numStrings; }

private:
    std::array<WaveguideString, kMaxStrings> strings;
    std::array<Exciter, kMaxStrings> exciters;
    std::array<float, kMaxStrings> gainLeft {}, gainRight {};
    ExciterSettings exciterSettings;
    int numStrings = 0;
    uint32_t rng = 0x9E3779B9u;

    float dcCoef = 0.999f;
    std::array<float, 2> dcX1 {}, dcY1 {};
};
} // namespace zither
