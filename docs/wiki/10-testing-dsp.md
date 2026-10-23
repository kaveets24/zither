# 10. Testing DSP

"It sounds right" doesn't survive refactoring. Zither checks its core behaviour with unit tests that **measure the
sound**, in [`tests/`](../../tests), using Catch2.

## Why the core has no JUCE

`Source/dsp/` and `Source/tuning/` are plain C++20 with no JUCE headers. They build into a small library,
`zither_core`, so the tests:

- build in seconds without compiling the plugin,
- run without a host, audio device or UI,
- and can feed exact inputs and inspect every output sample.

```sh
cmake --build build --target zither_tests -j && ./build/tests/zither_tests
```

## Measuring pitch: the Goertzel algorithm

To check that a string is in tune, the test renders 1.7 s of it and asks "how strong is frequency $f$ in this
signal?" for many candidate frequencies. A full FFT would be overkill. The **Goertzel algorithm** computes the
strength of **one** frequency with a tiny recursive filter:

```cpp
const double coeff = 2.0 * cos (2π * hz / sampleRate);
for (n ...) { s = x[n] * window + coeff * s1 - s2;  s2 = s1;  s1 = s; }
magnitude = sqrt (s1*s1 + s2*s2 - coeff*s1*s2);
```

`measureCentsOffset` searches ±40 cents around the expected pitch in 1-cent steps, then refines around the best
match in 0.05-cent steps. The **Hann window** tapers the analysed section's edges, so it doesn't smear energy into
neighbouring frequencies.

The test covers **every combination**, using Catch2's `GENERATE`:

```cpp
const double sampleRate = GENERATE (44100.0, 48000.0, 96000.0);
const double hz = GENERATE (65.41, 130.81, 440.0, 1046.5, 2093.0, 4186.0);
const float inharmonicity = GENERATE (0.0f, 0.5f, 1.0f);
...
REQUIRE (std::abs (cents) < 3.0);
```

3 × 6 × 3 = 54 cases, including the hardest: a 4186 Hz string with maximum stiffness at 44.1 kHz, where the whole
period is only 10.5 samples.

## Measuring decay

The decay test measures the fundamental's strength at 1 s and at 3 s. Over 2 s, a string with decay time T60 should
drop by $60 \cdot 2 / T_{60}$ dB. The test requires the measured drop to be within 15% of that.

## Stability and housekeeping

- **Stability:** render a very stiff 32.7 Hz string with both exciters, check every sample with `std::isfinite`, and
  check that the string has gone to sleep by the end.
- **Damping:** the effective T60 drops below 0.2 s when damped.
- **String bank:** only the strings in the tuning produce sound.
- **Tuning and strum** ([`TuningTests.cpp`](../../tests/TuningTests.cpp)): mode construction, key snapping, octave
  folding, and exact strum timings.

## Writing a new DSP test

1. Render the processor offline into a `std::vector<float>`. No real-time needed.
2. **Measure** a property you can state as a number: a frequency, a level in dB, a decay time, "never NaN".
3. Check it across sample rates. Bugs that confuse samples with seconds show up immediately at 96 kHz.
4. Give tolerances that match what a listener could hear (3 cents, 15% decay), not bit-exact output, so you can
   improve the DSP without rewriting tests.
