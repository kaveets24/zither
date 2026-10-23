# Zither: project guide

A physically modelled, open-tuned zither instrument (VST3 / AU / Standalone) built with JUCE 8.
Its north star is the feeling of Laraaji's *Flow Goes The Universe* (1992): weightless, blissful,
cascading shimmer over a drone, with notes that never quite stop ringing. Every decision, sonic or visual,
should make the player feel *immersed and safe to wander*.

## Core values: sound

These define the instrument. Don't trade them away for features.

1. **No wrong notes.** The instrument is a fixed bank of strings in an open/modal tuning. Keys snap to the
   nearest string and fold by octave. Any chromatic mode added later must be opt-in, never the default.
2. **Strings ring.** Note-off does not stop sound by default. Long decay (≈18 s on the low strings) is the
   baseline. Damping is an explicit choice (Release Damping, sustain pedal).
3. **Gentle by default.** Soft felt-hammer attack, no harsh transients, no aggressive distortion. Any saturation
   is subtle colour. The output soft-clip is a safety net, not a sound.
4. **Organic imperfection.** Humanized strum timing and velocity, slight string stiffness (inharmonicity).
   Nothing should sound machine-gunned or quantized.
5. **Space is part of the instrument.** The phaser → ping-pong delay → reverb tail is designed with the strings.
   Echoes darken and thin as they repeat, so feedback becomes a wash instead of building up.
6. **Modelled, not sampled.** Waveguide strings and synthesized exciters, so every strike is unique and responds
   to velocity, hardness and strike position.

## Core values: engineering

- **Realtime safety.** No allocation, locks, logging or I/O in `processBlock`. Size everything in `prepareToPlay`.
  `juce::ScopedNoDenormals` on the audio thread.
- **UI ↔ audio only through lock-free channels.** The UI reads `std::atomic` levels and strike counters, and sends
  strikes through an `AbstractFifo` (`ZitherAudioProcessor::strikeString`). The UI never touches DSP objects.
- **JUCE-free, tested DSP core.** `Source/dsp/` and `Source/tuning/` are plain C++20 with no JUCE includes. DSP
  changes keep the tests in `tests/` green and add tests for new behaviour. Pitch must stay within ±3 cents at
  44.1/48/96 kHz across the inharmonicity range.
- **Sample-rate independent.** Specify filters in Hz/seconds, never per sample.
- **Parameter IDs are a contract.** IDs in `Source/Parameters.h` are saved in user sessions. Never rename or remove
  one. Add new ones with a version hint.
- **Warnings-clean** in our own code (JUCE's recommended warning flags plus `-Wall -Wextra -Wshadow -Wconversion` on the core).

## Functionality (current)

| Area | What it does | Code |
|---|---|---|
| Strings | 12–36 waveguide strings: delay + Thiran fractional tuning + 4-stage dispersion allpass + two-point loss filter (T60 at f0 and at a high reference). Strings sleep when silent. | `dsp/WaveguideString`, `dsp/StringBank` |
| Exciters | Felt hammer (raised-cosine pulse, contact time capped to half a period) or pluck (filtered noise). Strike-position comb. Velocity controls brightness and level. | `dsp/Exciter` |
| Tuning | Root + mode (Lydian, Ionian, Mixolydian, Dorian, Aeolian, major/minor pentatonic, major triad, suspended), string count and lowest octave. Highest string ≤ MIDI 108. | `tuning/Tuning` |
| Strum | Each key sweeps 1–8 neighbouring strings up/down/alternating, with humanize, scheduled sample-accurately across blocks. | `tuning/Strummer` |
| Tail | Slow phaser → ping-pong delay (feedback band-limited 140 Hz–4.5 kHz) → Freeverb-style reverb → output gain → soft clip. | `fx/FxChain` |
| Playing | MIDI keys, sustain pedal, and dragging across the on-screen strings (the hand is the strummer). | `PluginProcessor`, `ui/StringField` |

## Visual identity

**Open water, made of waves.** The player floats in calm water. Above, the rippling underside of the surface is
layered waves that cross and drift in opposite directions. Below, soft swells roll slowly past and fade into the deep.
Light falls through in shafts and caustic ripples. The middle, where the strings hang, stays calm and clear. Waves
are anchored in px to the top and bottom edges, so they frame the strings at any window size. No solid walls or
blobs: everything is translucent water and light.

- **Clean and minimal interface.** The scene is the backdrop. Controls live on one frosted-glass panel, split into
  pages (Tune · Strike · String · Strum · Space) so only a handful of controls show at once. Thin ring knobs,
  lowercase letter-spaced labels, values in faint text. No boxes, bevels or skeuomorphic hardware.
- **Optional visual layer that follows the zither.** The strings are drawn across the water. When struck, they
  vibrate with their real decay, send ripples out from the strike point, and release bubbles. Root strings are
  tinted coral, as harps mark their C strings. The **visuals** toggle (saved with the session) freezes all
  animation for a calm, static, low-CPU view. The strings stay playable either way.
- **Motion has two speeds.** The world moves *slowly* (waves, light, drifting bubbles: periods of seconds to
  minutes). Only the strings shimmer quickly, and only while they sound.
- **Palette** (`ui/Palette.h`): abyss `#040f1c` · deep water `#0a2a3d` · lagoon `#12485a` · aqua `#7fe7d9` ·
  pearl `#e8f4f0` · coral `#e08aa6` · mist `#9fb8c4`.
- Animation runs from one 30 Hz editor timer. Keep paint cheap: straight lines for silent strings and fixed-size
  particle pools (no allocation per frame).

## Architecture

```
Source/
  PluginProcessor.*   APVTS, MIDI → Strummer → StringBank → FxChain; UI strike FIFO; level/strike atomics for UI
  PluginEditor.*      Lays out the scene, string field, header and control panel; drives the animation timer
  Parameters.h        Parameter IDs + layout (IDs are a contract)
  dsp/                JUCE-free: WaveguideString, Exciter, StringBank, Random
  tuning/             JUCE-free: Tuning, Strummer
  fx/                 FxChain (JUCE dsp)
  ui/                 Palette, AquaLookAndFeel, WaterScene (surface waves, light, swells),
                      StringField (strings visualizer + touch strumming), ControlPanel (paged controls)
tests/                Catch2 tests for the core
```

## Build and test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Zither_All -j        # also installs AU/VST3 to ~/Library/Audio/Plug-Ins
cmake --build build --target zither_tests -j && ./build/tests/zither_tests
auval -v aumu Zthr Flwi                           # AU validation
open build/Zither_artefacts/Release/Standalone/Zither.app
```

This machine has the Command Line Tools only (no Xcode.app), so the build uses the Makefile generator.

## Code style

JUCE house style: Allman braces, 4-space indent, a space before call parentheses (`foo (x)`), `camelCase`
members, `noexcept` on audio-path functions, and `static_cast` over C casts. Namespaces: `zither` (core, fx) and
`zither::ui`. Comments explain *why* (the physics or the design intent), not what.

## Roadmap

1. ✅ Scaffold, waveguide strings, tuned mapping + strum, phaser/delay/reverb
2. ✅ Aquatic UI: wave scene, string visualizer with touch strumming, paged minimal controls
3. Sympathetic bridge bus, shimmer reverb, magnetic pickup + saturation, drone strings, hammer roll
4. Mood presets, MPE/aftertouch swells, Scala `.scl` / just-intonation tunings
