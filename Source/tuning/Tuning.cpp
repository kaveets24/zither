#include "Tuning.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace zither
{
namespace
{
    struct ModeDefinition
    {
        const char* name;
        std::array<int, 7> degrees;
        int size;
    };

    constexpr std::array<ModeDefinition, static_cast<size_t> (Mode::numModes)> modes { {
        { "Lydian",            { 0, 2, 4, 6, 7, 9, 11 }, 7 },
        { "Ionian",            { 0, 2, 4, 5, 7, 9, 11 }, 7 },
        { "Mixolydian",        { 0, 2, 4, 5, 7, 9, 10 }, 7 },
        { "Dorian",            { 0, 2, 3, 5, 7, 9, 10 }, 7 },
        { "Aeolian",           { 0, 2, 3, 5, 7, 8, 10 }, 7 },
        { "Major Pentatonic",  { 0, 2, 4, 7, 9 },        5 },
        { "Minor Pentatonic",  { 0, 3, 5, 7, 10 },       5 },
        { "Major Triad",       { 0, 4, 7 },              3 },
        { "Suspended",         { 0, 2, 7 },              3 },
    } };

    const ModeDefinition& definition (Mode mode) noexcept
    {
        return modes[static_cast<size_t> (std::clamp (static_cast<int> (mode), 0, static_cast<int> (Mode::numModes) - 1))];
    }
} // namespace

const char* getModeName (Mode mode) noexcept
{
    return definition (mode).name;
}

void Tuning::set (const TuningSettings& settings) noexcept
{
    const auto& def = definition (settings.mode);
    const int root = ((settings.rootPitchClass % 12) + 12) % 12;
    const int base = 12 * (std::clamp (settings.lowestOctave, 0, 6) + 1) + root;
    const int count = std::clamp (settings.numStrings, 1, kMaxStrings);

    numStrings = 0;
    for (int i = 0; i < count; ++i)
    {
        const int note = base + 12 * (i / def.size) + def.degrees[static_cast<size_t> (i % def.size)];
        if (note > kHighestNote)
            break;

        notes[static_cast<size_t> (numStrings++)] = note;
    }
}

double Tuning::getStringFrequency (int index) const noexcept
{
    return 440.0 * std::pow (2.0, (getStringNote (index) - 69) / 12.0);
}

int Tuning::nearestString (int midiNote) const noexcept
{
    if (numStrings == 0)
        return -1;

    const int lowest = notes[0];
    const int highest = notes[static_cast<size_t> (numStrings - 1)];

    if (highest - lowest >= 12)
    {
        while (midiNote < lowest)
            midiNote += 12;
        while (midiNote > highest)
            midiNote -= 12;
    }

    int best = 0;
    for (int i = 1; i < numStrings; ++i)
        if (std::abs (notes[static_cast<size_t> (i)] - midiNote) < std::abs (notes[static_cast<size_t> (best)] - midiNote))
            best = i;

    return best;
}
} // namespace zither
