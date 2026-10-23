# 3. Filters

A **filter** changes a signal differently at different frequencies. In code, a digital filter is a
**difference equation**: each output sample is a weighted sum of recent inputs and outputs.

## Notation: $z^{-1}$ is "one sample ago"

DSP writes filters as **transfer functions** in $z$. Read $z^{-1}$ as "delay by one sample". For example

```math
H(z) = \frac{1 - p}{1 - p\,z^{-1}}
```

is the same thing as the code

```math
y[n] = (1 - p)\,x[n] + p\,y[n-1]
```

Everything to the right of the "=" is either the input now, or something remembered from the previous sample (the
filter's **state**). Filters that feed their output back are **IIR** (infinite impulse response); filters that only
use past inputs are **FIR** (finite impulse response).

## Frequency response: magnitude and phase

Feed a sine at frequency $\omega$ into a linear filter and you get the same sine out, changed in two ways:

- **Magnitude** $|H(\omega)|$: how much louder or quieter it gets.
- **Phase** $\angle H(\omega)$: how much it gets shifted in time.

Phase converts into a time delay, the **phase delay**, in samples:

```math
\tau(\omega) = -\frac{\angle H(e^{j\omega})}{\omega}
```

This matters a lot for the string: **every filter inside the string loop adds delay, and delay changes pitch.**
`WaveguideString.cpp` computes the phase delay of each filter at the fundamental (`allpassPhaseDelay`,
`onePoleResponse`) and shortens the delay line to compensate.

## One-pole lowpass

The simplest useful filter: move a fraction of the way toward the input each sample.

```cpp
// FxChain::OnePole
float process (float x) noexcept { return state += coef * (x - state); }
coef = 1 - exp(-2π · cutoffHz / sampleRate)
```

It smooths out fast changes (high frequencies) and passes slow ones (low frequencies). The slope is gentle:
−6 dB per octave above the cutoff.

**Where it's used:**
- The **loss filter** inside every string, which makes high partials die faster than low ones
  ([The waveguide string](05-the-waveguide-string.md))
- The delay's feedback path, which darkens each echo ([The effects tail](08-the-effects-tail.md))
- The pluck exciter, which colours its noise burst ([Exciters](06-exciters.md))

## Highpass by subtraction

Whatever a lowpass removes is the high part, so `x − lowpass(x)` is a highpass. `FxChain::feedbackTone` chains both:

```cpp
const float lp = toneLowpass[channel].process (x);       // remove above 4.5 kHz
return lp - toneHighpass[channel].process (lp);          // then remove below 140 Hz
```

Together they make a **band-pass** filter: each echo loses both its sparkle and its boom.

## DC blocker

**DC** is a constant offset (0 Hz). It's inaudible, but it wastes headroom and can push signals into clipping. A
string loop passes DC with almost no loss, so any offset would circulate for a long time. `StringBank::render` ends
with a DC blocker, a highpass at about 10 Hz:

```math
y[n] = x[n] - x[n-1] + R\,y[n-1], \qquad R = e^{-2\pi \cdot 10 / f_s}
```

## Allpass filters

An **allpass** has magnitude exactly 1 at every frequency: it changes no frequency's loudness, **only its delay**.
The first-order allpass used all over this code:

```math
H(z) = \frac{a + z^{-1}}{1 + a\,z^{-1}}
```

```cpp
y = a * x + x1 - a * y1;   // x1, y1 = previous input and output
```

Its delay at DC is $(1 - a)/(1 + a)$ samples, and it changes with frequency depending on $a$. That one property gives
three different tools:

| Use | How | Page |
|---|---|---|
| **Fractional delay** (Thiran) | Choose $a$ so the delay is, e.g., 0.73 samples | [Delay lines](04-delay-lines.md) |
| **Dispersion** (stiff strings) | Negative $a$ delays low frequencies more than high | [The waveguide string](05-the-waveguide-string.md) |
| **Phaser** | Sweep the allpass frequency and mix with the dry signal | [The effects tail](08-the-effects-tail.md) |

## Comb filters

Add a signal to a delayed copy of itself, and frequencies whose period fits the delay reinforce or cancel. The
frequency response looks like the teeth of a comb.

- **Feedforward comb**, $y[n] = x[n] - x[n-b]$: notches at every multiple of $f_s / b$. The exciter uses this to
  model **where** the string is struck ([Exciters](06-exciters.md)).
- **Feedback comb**, $y[n] = x[n] + g\,y[n-D]$: peaks at every multiple of $f_s / D$, which ring for a long time if $g$ is
  close to 1. **That is a plucked string.** Freeverb is also built from 8 of them.

## Try it

- Sketch $y[n] = 0.5\,x[n] + 0.5\,y[n-1]$ responding to an input of 1, 0, 0, 0, … (an impulse). That shape is the
  impulse response.
- For the allpass above, work out the DC delay for $a = 0.5$, $a = 0$ and $a = -0.5$.
