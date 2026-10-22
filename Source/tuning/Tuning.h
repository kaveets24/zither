#pragma once

#include <array>

namespace zither
{
enum class Mode : int
{
    Lydian = 0,
    Ionian,
    Mixolydian,
    Dorian,
    Aeolian,
    MajorPentatonic,
    MinorPentatonic,
    MajorTriad,
    Suspended,
    numModes
};

const char* getModeName (Mode) noexcept;

struct TuningSettings
{
    int rootPitchClass = 0; // 0 = C
    Mode mode = Mode::Lydian;
    int numStrings = 30;
    int lowestOctave = 2;   // octave of the lowest string, MIDI convention (C2 = 36)

    bool operator== (const TuningSettings&) const = default;
};

/**
    A fixed bank of strings tuned to one mode, ascending from the root. Incoming keys snap to the
    nearest string, so every note played falls inside the open tuning.
*/
class Tuning
{
public:
    static constexpr int kMaxStrings = 36;
    static constexpr int kHighestNote = 108;

    void set (const TuningSettings&) noexcept;

    int getNumStrings() const noexcept { return numStrings; }
    int getStringNote (int index) const noexcept { return notes[static_cast<size_t> (index)]; }
    double getStringFrequency (int index) const noexcept;

    /** Index of the string closest to the key. Out-of-range keys fold by octaves. Ties pick the lower string. */
    int nearestString (int midiNote) const noexcept;

private:
    std::array<int, kMaxStrings> notes {};
    int numStrings = 0;
};
} // namespace zither
