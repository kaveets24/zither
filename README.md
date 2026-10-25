# Zither Aquatica

An open-tuned, physically modelled zither instrument (VST3 / AU / Standalone) built with JUCE.
The sound it is reaching for is Laraaji's *Flow Goes The Universe* (1992): weightless, cascading shimmer
over a drone, with strings that never quite stop ringing.

## Build

Requirements: CMake ≥ 3.22 and the Xcode Command Line Tools (or full Xcode). JUCE 8 and Catch2 are fetched automatically.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Zither_All -j      # VST3, AU, Standalone; installs to ~/Library/Audio/Plug-Ins
cmake --build build --target zither_tests zither_plugin_tests -j && ctest --test-dir build
```

With full Xcode installed, `cmake -B build -G Xcode` gives you an Xcode project instead.

Builds are universal (Apple Silicon + Intel) by default. For faster local builds, configure with
`-DCMAKE_OSX_ARCHITECTURES=arm64`. CI builds on macOS (universal) and Windows (x64) for every push and PR, then runs
the unit tests, [pluginval](https://github.com/Tracktion/pluginval) at strictness 10 and, on macOS, `auval`.

- Standalone app: `build/Zither_artefacts/Release/Standalone/Zither Aquatica.app`
- AU validation: `auval -v aumu Zaqa Skvt`. If a DAW doesn't see a fresh build, run `killall -9 AudioComponentRegistrar`.

### Windows

Requirements: CMake ≥ 3.22 and Visual Studio 2022 or later with the *Desktop development with C++* workload.
Windows builds VST3 and Standalone (AU is macOS-only), with the MSVC runtime linked statically, so users don't need
a VC++ redistributable.

```sh
cmake -B build
cmake --build build --config Release --target Zither_All --parallel
cmake --build build --config Release --target zither_tests zither_plugin_tests --parallel && ctest --test-dir build -C Release
```

The build doesn't install the plugin, because the system VST3 folder needs admin rights. Copy
`build/Zither_artefacts/Release/VST3/Zither Aquatica.vst3` into `C:\Program Files\Common Files\VST3`. No Windows machine?
Every CI run uploads a `Zither-Aquatica-VST3-Windows-x64` artifact that has passed pluginval.

## Realtime safety

Zither follows standard audio-plugin practice: **nothing on the audio thread may block.** The host gives `processBlock`
a hard deadline for every buffer (2.7 ms at 48 kHz with 128-sample blocks). Missing it even once is heard as a loud
click, pop or dropout, so anything that can take an unpredictable amount of time is forbidden there:

- no memory allocation or freeing (`new`, growing a `std::vector`, building a `juce::String`)
- no locks or waiting on other threads (mutexes, `CriticalSection`, condition variables)
- no file, network or console I/O, including logging and `DBG`
- no calls into the GUI or message thread, and no unbounded loops

Every buffer is sized in `prepareToPlay`. The UI and audio threads talk only through lock-free channels: atomics for
levels and parameters, and an `AbstractFifo` for on-screen strikes (if it's full, a strike is dropped, not waited on).
`juce::ScopedNoDenormals` prevents the CPU spikes that denormals cause in long, decaying tails. CI runs pluginval at
strictness 10 on every change.

The rules for contributors are in [CLAUDE.md](CLAUDE.md). The wiki's [realtime safety](docs/wiki/09-realtime-safety.md)
page explains why they matter.

## How it gets the sound

| Idea | Where |
|---|---|
| **Waveguide strings** (extended Karplus-Strong): delay line + Thiran fractional tuning + allpass dispersion for steel-string stiffness + a two-point loss filter. All filter phase delays are compensated at f0, so strings stay in tune. | `Source/dsp/WaveguideString.*` |
| **Hammer and pluck exciters**: a raised-cosine felt-mallet pulse or filtered noise, shaped by a strike-position comb. Velocity controls brightness as well as level. | `Source/dsp/Exciter.*` |
| **Open tuning, every string available**: 12–36 strings in a mode (Lydian by default). Keys snap to the nearest string and fold by octave, so there are no wrong notes. Strings ring until they decay; release damping is optional. | `Source/tuning/Tuning.*`, `Source/dsp/StringBank.*` |
| **Strum / sweep**: each key sweeps across neighbouring strings (up, down or alternating) with humanized timing and velocity, scheduled sample-accurately. | `Source/tuning/Strummer.*` |
| **Electric-zither tail**: slow phaser → filtered ping-pong delay (each echo darker) → large reverb. | `Source/fx/FxChain.*` |

The DSP core (`dsp/`, `tuning/`) is plain C++20 with no JUCE dependency, and is unit tested in `tests/`.

New to plugin DSP? The [DSP wiki](docs/wiki/README.md) explains every concept the instrument uses, from samples and
filters to waveguide strings, with links into the code.

## Interface

Open water under a rippling surface, with slow swells rolling past below. The strings hang in the water: they vibrate as they
ring, send ripples and bubbles from where they were struck, and can be played by dragging across them.
One frosted-glass panel holds paged controls (tune · strike · string · strum · space). The **visuals** toggle
freezes the scene into a still, low-CPU frame. See [CLAUDE.md](CLAUDE.md) for the design language and core values.

## Roadmap

1. ✅ Scaffold, waveguide string bank, tuned-zither mapping + strum, phaser/delay/reverb
2. ✅ Aquatic UI: wave scene, string visualizer with touch strumming, paged minimal controls
3. Sympathetic bridge bus (undamped strings ring in sympathy), shimmer reverb (octave-up in the feedback loop), magnetic pickup + saturation, self-sounding drone strings, hammer-roll / tremolo gesture
4. Mood presets, MPE/aftertouch swells
5. Scala `.scl` import, just intonation and microtonal tunings

## License

Zither Aquatica is free software under the [GNU Affero General Public License v3.0](LICENSE). It is built on
[JUCE](https://juce.com), which it uses under JUCE's AGPLv3 licence. If you distribute a modified build, you must make
its source available under the same licence.
