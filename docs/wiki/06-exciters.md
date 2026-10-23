# 6. Exciters

The [waveguide string](05-the-waveguide-string.md) is a resonator: it rings, but something has to start it. The
**exciter** generates a short burst (a few milliseconds) and adds it into the loop:

```cpp
const float out = lossState + excitation;   // WaveguideString::processSample
```

Everything about the attack (how soft, how bright, how loud) comes from that burst. Code:
[`Source/dsp/Exciter.cpp`](../../Source/dsp/Exciter.cpp).

## Felt hammer: a raised-cosine pulse

A felt mallet presses into the string and springs back. The force rises and falls smoothly, which the code models
with a **raised cosine** (one hump of a cosine wave):

```cpp
raw[n] = 0.5 * (1.0 - cos (2π (n + 0.5) / rawLength));
```

```text
force
  ^      .-''-.
  |    .'      '.
  |  .'          '.
  +--'-------------'---> time
     |<- contact ->|
```

### Contact time sets brightness

The key idea is the **time–frequency trade-off**: a short pulse has a wide spectrum, and a long pulse has a narrow
one. A raised cosine of length $T$ has its first spectral null at $2/T$:

| Contact time | Mostly below | Sounds like |
|---|---|---|
| 4 ms (softest) | ~500 Hz | Round, dark, felt |
| 0.35 ms (hardest) | ~5.7 kHz | Bright, metallic |

Contact time comes from a blend of **Hardness** and **velocity**, because real hammers hit harder strikes faster and
compress the felt less:

```cpp
const double bright = clamp (0.2 + 0.5 * hardness + 0.3 * vel, 0.0, 1.0);
widthMs = 4.0 * pow (0.35 / 4.0, bright);   // 4 ms ... 0.35 ms on a log scale
```

Two physical touches:
- **Short strings push the hammer back sooner**, so contact is capped at 0.6 of the string's period. Otherwise a slow
  hammer would cancel its own treble notes.
- **Narrow pulses carry less energy**, so the code boosts them a little (`amp *= pow (4 / widthMs, 0.35)`) so
  hard hits aren't quieter.

## Pluck: filtered noise

The **Pluck** exciter goes back to Karplus–Strong: one period of random noise, coloured by a one-pole lowpass
(darker when soft), with its average (DC) removed and its peak normalised. Because the noise is random, **every
pluck is slightly different**, which is one of the "organic imperfection" values of this instrument.

The random numbers come from `nextBipolarRandom` in [`Random.h`](../../Source/dsp/Random.h), a tiny *xorshift*
generator. It's deterministic and never allocates, so it is safe on the audio thread (unlike `std::random_device`).

## Strike position: a comb filter

Where you strike a string changes its tone. Striking at a point $\beta$ of the way along the string (for example 1/2
= the middle) **can't excite any harmonic that has a node (a still point) there**:

| Strike position $\beta$ | Missing harmonics | Character |
|---|---|---|
| 1/2 (middle) | 2, 4, 6, … (all even) | Hollow, soft |
| 1/4 | 4, 8, 12, … | Fuller |
| 0.13 (default, near the bridge) | ~every 7th–8th | Bright, complete |
| 0.02 | Only very high ones | Thin, twangy |

The exciter models this with a **feedforward comb** (see [Filters](03-filters.md)): the burst minus a copy of itself
delayed by $\beta P$ samples, representing the wave that reflects off the near end:

```cpp
const int combDelay = lround (strikePosition * periodSamples);
burst[n] = scale * (raw[n] - raw[n - combDelay]);
```

A useful bonus: $e[n] - e[n-b]$ **always sums to zero**, so no DC offset gets trapped in the string's feedback loop.

## Velocity

Velocity affects the sound twice: it makes the strike **louder** (`0.05 + 0.95 · v^1.5`) and **brighter** (via contact
time). Playing harder doesn't just turn the volume up, which is a big part of why a modelled instrument feels alive.

## Try it

- Play one note at Hardness 0% and then 100% and listen to the attack.
- Sweep Strike Position from 2% to 50% while repeatedly hitting one low string. At 50% it should sound hollow,
  like a clarinet.
- Switch to Pluck and hit the same key repeatedly. Can you hear that no two plucks are identical?
