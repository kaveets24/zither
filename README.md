# Zither

An open-tuned, physically modelled zither instrument (VST3 / AU / Standalone) built with JUCE.
The sound it is reaching for is Laraaji's *Flow Goes The Universe* (1992): weightless, cascading shimmer
over a drone, with strings that never quite stop ringing.

## Build

Requirements: CMake ≥ 3.22 and the Xcode Command Line Tools (or full Xcode). JUCE 8 and Catch2 are fetched automatically.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Zither_All -j      # VST3, AU, Standalone; installs to ~/Library/Audio/Plug-Ins
cmake --build build --target zither_tests -j && ctest --test-dir build
```

With full Xcode installed, `cmake -B build -G Xcode` gives you an Xcode project instead.

- Standalone app: `build/Zither_artefacts/Release/Standalone/Zither.app`
- AU validation: `auval -v aumu Zthr Flwi`. If a DAW doesn't see a fresh build, run `killall -9 AudioComponentRegistrar`.
- The manufacturer code `Flwi` and company name in `CMakeLists.txt` are placeholders. Change them before distributing.

## How it gets the sound

| Idea | Where |
|---|---|
| **Waveguide strings** (extended Karplus-Strong): delay line + Thiran fractional tuning + allpass dispersion for steel-string stiffness + a two-point loss filter. All filter phase delays are compensated at f0, so strings stay in tune. | `Source/dsp/WaveguideString.*` |
| **Hammer and pluck exciters**: a raised-cosine felt-mallet pulse or filtered noise, shaped by a strike-position comb. Velocity controls brightness as well as level. | `Source/dsp/Exciter.*` |
| **Open tuning, every string available**: 12–36 strings in a mode (Lydian by default). Keys snap to the nearest string and fold by octave, so there are no wrong notes. Strings ring until they decay; release damping is optional. | `Source/tuning/Tuning.*`, `Source/dsp/StringBank.*` |
| **Strum / sweep**: each key sweeps across neighbouring strings (up, down or alternating) with humanized timing and velocity, scheduled sample-accurately. | `Source/tuning/Strummer.*` |
| **Electric-zither tail**: slow phaser → filtered ping-pong delay (each echo darker) → large reverb. | `Source/fx/FxChain.*` |

The DSP core (`dsp/`, `tuning/`) is plain C++20 with no JUCE dependency, and is unit tested in `tests/`.

## Interface

An underwater grotto with slowly melting violet walls. The strings hang in the water: they vibrate as they
ring, send ripples and bubbles from where they were struck, and can be played by dragging across them.
One frosted-glass panel holds paged controls (tune · strike · string · strum · space). The **visuals** toggle
freezes the scene into a still, low-CPU frame. See [CLAUDE.md](CLAUDE.md) for the design language and core values.

## Roadmap

1. ✅ Scaffold, waveguide string bank, tuned-zither mapping + strum, phaser/delay/reverb
2. ✅ Aquatic UI: melting-wall scene, string visualizer with touch strumming, paged minimal controls
3. Sympathetic bridge bus (undamped strings ring in sympathy), shimmer reverb (octave-up in the feedback loop), magnetic pickup + saturation, self-sounding drone strings, hammer-roll / tremolo gesture
4. Mood presets, MPE/aftertouch swells
5. Scala `.scl` import, just intonation and microtonal tunings
