# Glossary

| Term | Meaning | See |
|---|---|---|
| **Aliasing** | Frequencies above Nyquist folding back as false, inharmonic tones | [2](02-digital-audio-basics.md) |
| **Allpass filter** | A filter with gain 1 at every frequency that only changes delay (phase) | [3](03-filters.md) |
| **APVTS** | `AudioProcessorValueTreeState`: JUCE's container for parameters and their saved state | [1](01-how-a-plugin-works.md) |
| **Block** | The chunk of samples the host asks for in one `processBlock` call | [1](01-how-a-plugin-works.md) |
| **Cent** | 1/100 of a semitone; $1200 \log_2(f_1/f_2)$ | [2](02-digital-audio-basics.md) |
| **Circular buffer** | An array with a wrapping write position, used for delay lines | [4](04-delay-lines.md) |
| **Comb filter** | A signal plus a delayed copy, giving evenly spaced peaks or notches | [3](03-filters.md) |
| **DC** | 0 Hz; a constant offset in the signal | [3](03-filters.md) |
| **Decibel (dB)** | Logarithmic level: $20 \log_{10}(a)$ | [2](02-digital-audio-basics.md) |
| **Denormal** | A tiny float near zero that some CPUs process very slowly | [9](09-realtime-safety.md) |
| **Digital waveguide** | A delay line plus filters in a feedback loop that models waves travelling along a string | [5](05-the-waveguide-string.md) |
| **Dispersion** | Different frequencies travelling at different speeds; in strings, it's caused by stiffness | [5](05-the-waveguide-string.md) |
| **Exciter** | The short burst that starts a string ringing (hammer or pluck) | [6](06-exciters.md) |
| **FIFO** | First-in, first-out queue; `AbstractFifo` is JUCE's lock-free one | [9](09-realtime-safety.md) |
| **FIR / IIR** | Filters without / with feedback of their own output | [3](03-filters.md) |
| **Fractional delay** | A delay that isn't a whole number of samples | [4](04-delay-lines.md) |
| **Freeverb** | A classic reverb design: 8 feedback combs and 4 allpasses per channel | [8](08-the-effects-tail.md) |
| **Fundamental ($f_0$)** | The lowest frequency of a note, which we hear as its pitch | [2](02-digital-audio-basics.md) |
| **Goertzel algorithm** | A cheap way to measure the strength of one frequency in a signal | [10](10-testing-dsp.md) |
| **Host** | The app that loads plugins (a DAW, or the Standalone wrapper) | [1](01-how-a-plugin-works.md) |
| **Inharmonicity** | Partials sitting slightly sharp of exact multiples of $f_0$ | [5](05-the-waveguide-string.md) |
| **Karplus–Strong** | The 1983 plucked-string algorithm: noise looped through a delay and an averaging filter | [5](05-the-waveguide-string.md) |
| **LFO** | Low-frequency oscillator: a slow wave that modulates a parameter (e.g. the phaser sweep) | [8](08-the-effects-tail.md) |
| **Mode** | A set of scale degrees above a root (Lydian, Dorian, …) | [7](07-tuning-and-strumming.md) |
| **Nyquist frequency** | Half the sample rate; the highest frequency that can be represented | [2](02-digital-audio-basics.md) |
| **Partial / overtone / harmonic** | The frequencies above the fundamental that make up a tone's timbre | [5](05-the-waveguide-string.md) |
| **Period ($P$)** | Length of one cycle; in samples, $f_s / f_0$ | [2](02-digital-audio-basics.md) |
| **Phase delay** | How many samples a filter delays a given frequency | [3](03-filters.md) |
| **Phaser** | An effect made by mixing a signal with an allpass-swept copy, giving moving notches | [8](08-the-effects-tail.md) |
| **Ping-pong delay** | A stereo echo whose repeats alternate left and right | [8](08-the-effects-tail.md) |
| **Sample rate ($f_s$)** | Samples per second (44.1k, 48k, 96k, …) | [2](02-digital-audio-basics.md) |
| **Smoothing** | Gliding a parameter to its new value to avoid clicks and zipper noise | [8](08-the-effects-tail.md) |
| **Soft clip** | A gentle curve that limits peaks without harsh distortion | [8](08-the-effects-tail.md) |
| **T60** | The time for a sound to decay by 60 dB (to 1/1000 of its amplitude) | [5](05-the-waveguide-string.md) |
| **Thiran allpass** | An allpass filter designed for a specific fractional delay | [4](04-delay-lines.md) |
| **Transfer function ($H(z)$)** | A filter written in terms of $z^{-1}$ (one-sample delays) | [3](03-filters.md) |
| **Velocity** | How hard a key was struck (MIDI 0–127; 0–1 in JUCE) | [6](06-exciters.md) |
| **Zipper noise** | Buzzy stepping heard when a parameter jumps in small increments | [8](08-the-effects-tail.md) |
