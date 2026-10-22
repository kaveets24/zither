#pragma once

#include <cstdint>

namespace zither
{
/** xorshift32: tiny, allocation-free and deterministic, so it is safe on the audio thread. */
inline float nextBipolarRandom (uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return static_cast<float> (state >> 8) / 8388608.0f - 1.0f; // [-1, 1)
}
} // namespace zither
