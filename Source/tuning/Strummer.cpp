#include "Strummer.h"

#include <limits>

namespace zither
{
void Strummer::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    numPending = 0;
    now = 0;
}

int Strummer::samplesUntilNext() const noexcept
{
    if (numPending == 0)
        return -1;

    auto next = std::numeric_limits<int64_t>::max();
    for (int i = 0; i < numPending; ++i)
        next = std::min (next, pending[static_cast<size_t> (i)].time);

    return static_cast<int> (std::clamp<int64_t> (next - now, 0, std::numeric_limits<int>::max()));
}

void Strummer::schedule (int64_t time, int stringIndex, float velocity) noexcept
{
    if (numPending < kCapacity)
        pending[static_cast<size_t> (numPending++)] = { time, stringIndex, velocity };
}
} // namespace zither
