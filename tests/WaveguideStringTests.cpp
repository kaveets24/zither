#include "dsp/Exciter.h"
#include "dsp/StringBank.h"
#include "dsp/WaveguideString.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

using namespace zither;

namespace
{
constexpr double kPi = 3.14159265358979323846;

std::vector<float> renderString (double sampleRate, double hz, const StringParams& params, double seconds,
                                 ExciterType type = ExciterType::Hammer, WaveguideString* stringOut = nullptr)
{
    WaveguideString string;
    string.prepare (sampleRate, 30.0);
    string.setParams (params);
    string.setFrequency (hz);

    Exciter exciter;
    exciter.prepare (sampleRate, static_cast<int> (sampleRate / 30.0) + 8);
    uint32_t rng = 12345;
    exciter.trigger ({ type, 0.4f, 0.13f }, 0.8f, string.getPeriodSamples(), rng);
    string.wake();

    std::vector<float> out (static_cast<size_t> (seconds * sampleRate));
    for (auto& s : out)
        s = string.isActive() ? string.processSample (exciter.nextSample()) : 0.0f;

    if (stringOut != nullptr)
        *stringOut = string;

    return out;
}

/** Magnitude of the Hann-windowed DTFT at one frequency (Goertzel). */
double magnitudeAt (const std::vector<float>& x, size_t start, size_t length, double sampleRate, double hz)
{
    const double coeff = 2.0 * std::cos (2.0 * kPi * hz / sampleRate);
    double s1 = 0.0, s2 = 0.0;
    for (size_t n = 0; n < length; ++n)
    {
        const double window = 0.5 - 0.5 * std::cos (2.0 * kPi * static_cast<double> (n) / static_cast<double> (length - 1));
        const double s = x[start + n] * window + coeff * s1 - s2;
        s2 = s1;
        s1 = s;
    }
    return std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - coeff * s1 * s2));
}

/** Finds the fundamental near `expectedHz` and returns its offset in cents. */
double measureCentsOffset (const std::vector<float>& x, double sampleRate, double expectedHz)
{
    const auto start = static_cast<size_t> (0.1 * sampleRate);
    const auto length = static_cast<size_t> (1.5 * sampleRate);

    auto search = [&] (double from, double to, double step) {
        double best = from, bestMag = -1.0;
        for (double c = from; c <= to; c += step)
        {
            const double mag = magnitudeAt (x, start, length, sampleRate, expectedHz * std::pow (2.0, c / 1200.0));
            if (mag > bestMag)
            {
                bestMag = mag;
                best = c;
            }
        }
        return best;
    };

    const double coarse = search (-40.0, 40.0, 1.0);
    return search (coarse - 1.0, coarse + 1.0, 0.05);
}
} // namespace

TEST_CASE ("String fundamental is in tune", "[string][pitch]")
{
    const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
    const double hz = GENERATE (65.41, 130.81, 440.0, 1046.5, 2093.0, 4186.0);
    const float inharmonicity = GENERATE (0.0f, 0.5f, 1.0f);

    StringParams params;
    params.decaySeconds = 20.0f;
    params.inharmonicity = inharmonicity;

    const auto out = renderString (sampleRate, hz, params, 1.7);
    const double cents = measureCentsOffset (out, sampleRate, hz);

    INFO ("sr=" << sampleRate << " hz=" << hz << " inharm=" << inharmonicity << " offset=" << cents << " cents");
    REQUIRE (std::abs (cents) < 3.0);
}

TEST_CASE ("String decays at the requested T60", "[string][decay]")
{
    constexpr double sampleRate = 48000.0;
    constexpr double hz = 220.0;

    StringParams params;
    params.decaySeconds = 6.0f;
    params.brightness = 1.0f;
    params.inharmonicity = 0.0f;

    WaveguideString string;
    const auto out = renderString (sampleRate, hz, params, 4.0, ExciterType::Hammer, &string);

    const auto window = static_cast<size_t> (0.25 * sampleRate);
    const double early = magnitudeAt (out, static_cast<size_t> (1.0 * sampleRate), window, sampleRate, hz);
    const double late = magnitudeAt (out, static_cast<size_t> (3.0 * sampleRate), window, sampleRate, hz);

    const double measuredDropDb = 20.0 * std::log10 (early / late);
    const double expectedDropDb = 60.0 * 2.0 / string.getEffectiveT60();

    INFO ("measured " << measuredDropDb << " dB, expected " << expectedDropDb << " dB");
    REQUIRE_THAT (measuredDropDb, Catch::Matchers::WithinRel (expectedDropDb, 0.15));
}

TEST_CASE ("String output stays finite and the string goes to sleep", "[string][stability]")
{
    const auto type = GENERATE (ExciterType::Hammer, ExciterType::Pluck);

    StringParams params;
    params.decaySeconds = 1.0f;
    params.inharmonicity = 1.0f;

    WaveguideString string;
    const auto out = renderString (44100.0, 32.7, params, 12.0, type, &string);

    for (const auto s : out)
        REQUIRE (std::isfinite (s));

    REQUIRE_FALSE (string.isActive());
}

TEST_CASE ("Damping shortens the ring", "[string][damping]")
{
    StringParams params;
    params.decaySeconds = 20.0f;
    params.damping = 1.0f;

    WaveguideString string;
    string.prepare (48000.0, 30.0);
    string.setParams (params);
    string.setFrequency (220.0);
    const double open = string.getEffectiveT60();

    string.setDamped (true);
    REQUIRE (string.getEffectiveT60() < 0.2);
    REQUIRE (open > 10.0);
}

TEST_CASE ("String bank renders only tuned strings", "[bank]")
{
    Tuning tuning;
    tuning.set ({ 0, Mode::Lydian, 12, 2 });

    StringBank bank;
    bank.prepare (44100.0);
    bank.setTuning (tuning);
    REQUIRE (bank.getNumStrings() == 12);

    bank.excite (40, 1.0f); // out of range: ignored
    std::vector<float> left (512, 0.0f), right (512, 0.0f);
    bank.render (left.data(), right.data(), 512);
    for (const auto s : left)
        REQUIRE (s == 0.0f);

    bank.excite (5, 1.0f);
    bank.render (left.data(), right.data(), 512);
    float peak = 0.0f;
    for (const auto s : left)
        peak = std::max (peak, std::abs (s));
    REQUIRE (peak > 0.001f);
}
