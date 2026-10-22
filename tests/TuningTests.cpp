#include "tuning/Strummer.h"
#include "tuning/Tuning.h"

#include <catch2/catch_test_macros.hpp>

#include <utility>
#include <vector>

using namespace zither;

TEST_CASE ("C Lydian strings ascend through the mode", "[tuning]")
{
    Tuning tuning;
    tuning.set ({ 0, Mode::Lydian, 30, 2 });

    REQUIRE (tuning.getNumStrings() == 30);
    REQUIRE (tuning.getStringNote (0) == 36); // C2
    REQUIRE (tuning.getStringNote (3) == 42); // F#2, the Lydian fourth
    REQUIRE (tuning.getStringNote (7) == 48); // C3
    REQUIRE (tuning.getStringNote (29) == 86);
}

TEST_CASE ("Root transposes the whole bank", "[tuning]")
{
    Tuning tuning;
    tuning.set ({ 2, Mode::MajorPentatonic, 10, 2 }); // D major pentatonic from D2

    REQUIRE (tuning.getStringNote (0) == 38);
    REQUIRE (tuning.getStringNote (5) == 50);
}

TEST_CASE ("Keys snap to the nearest string", "[tuning]")
{
    Tuning tuning;
    tuning.set ({ 0, Mode::Lydian, 30, 2 });

    REQUIRE (tuning.nearestString (36) == 0);
    REQUIRE (tuning.nearestString (37) == 0); // tie between C and D -> lower
    REQUIRE (tuning.nearestString (41) == 2); // F -> tie between E and F# -> lower (E)
    REQUIRE (tuning.nearestString (42) == 3);
    REQUIRE (tuning.nearestString (48) == 7);
}

TEST_CASE ("Out-of-range keys fold by octave", "[tuning]")
{
    Tuning tuning;
    tuning.set ({ 0, Mode::Lydian, 30, 2 });

    REQUIRE (tuning.nearestString (24) == 0);                           // C1 -> C2
    REQUIRE (tuning.getStringNote (tuning.nearestString (120)) % 12 == 0); // C9 folds to a C string
}

TEST_CASE ("Strings above the highest note are dropped", "[tuning]")
{
    Tuning tuning;
    tuning.set ({ 0, Mode::MajorTriad, 36, 2 });

    REQUIRE (tuning.getNumStrings() < 36);
    REQUIRE (tuning.getStringNote (tuning.getNumStrings() - 1) <= Tuning::kHighestNote);
}

namespace
{
std::vector<std::pair<int64_t, int>> runStrum (Strummer& strummer, int stringIndex, int numStrings)
{
    std::vector<std::pair<int64_t, int>> hits;
    int64_t t = 0;
    auto record = [&] (int s, float) { hits.emplace_back (t, s); };

    strummer.noteOn (stringIndex, 1.0f, numStrings, record);
    for (;;)
    {
        strummer.popDue (record);
        const int wait = strummer.samplesUntilNext();
        if (wait < 0)
            break;

        strummer.advance (wait);
        t += wait;
    }
    return hits;
}
} // namespace

TEST_CASE ("Strum sweeps upward at the set interval", "[strum]")
{
    Strummer strummer;
    strummer.prepare (48000.0);
    strummer.setSettings ({ true, 10.0f, 4, StrumDirection::Up, 0.0f });

    const auto hits = runStrum (strummer, 5, 30);
    const std::vector<std::pair<int64_t, int>> expected { { 0, 5 }, { 480, 6 }, { 960, 7 }, { 1440, 8 } };
    REQUIRE (hits == expected);
}

TEST_CASE ("Downward strum stops at the lowest string", "[strum]")
{
    Strummer strummer;
    strummer.prepare (48000.0);
    strummer.setSettings ({ true, 10.0f, 4, StrumDirection::Down, 0.0f });

    const auto hits = runStrum (strummer, 1, 30);
    const std::vector<std::pair<int64_t, int>> expected { { 0, 1 }, { 480, 0 } };
    REQUIRE (hits == expected);
}

TEST_CASE ("Strum disabled plays a single string", "[strum]")
{
    Strummer strummer;
    strummer.prepare (48000.0);
    strummer.setSettings ({ false, 10.0f, 4, StrumDirection::Up, 0.0f });

    const auto hits = runStrum (strummer, 5, 30);
    REQUIRE (hits.size() == 1);
}
