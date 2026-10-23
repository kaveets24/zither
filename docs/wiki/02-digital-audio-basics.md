# 2. Digital audio basics

## Samples and sample rate

Sound is air pressure changing over time. A computer stores it as **samples**: numbers measured at regular intervals.
The **sample rate** $f_s$ is how many samples per second. Common values are 44.1 kHz, 48 kHz and 96 kHz. Zither must
sound identical at all of them.

Samples are `float`s, nominally between −1 and +1. Values beyond that clip at the converter, which is why Zither ends
with a soft clipper.

## Nyquist: the highest frequency you can represent

A sample rate of $f_s$ can only represent frequencies below $f_s / 2$, the **Nyquist frequency**. Anything above it
folds back down as a false, inharmonic tone. This is called **aliasing**.

In the code: `WaveguideString::setFrequency` clamps every string to at most `0.45 * sampleRate`, safely below
Nyquist, and the highest string is MIDI 108 (4186 Hz).

## Frequency and period

A tone at frequency $f_0$ (in Hz) repeats every $1/f_0$ seconds. Measured in samples, the **period** is

```math
P = \frac{f_s}{f_0}
```

At 48 kHz, A3 (220 Hz) has a period of 218.18… samples. Periods are almost never whole numbers, which is the reason
for [fractional delay](04-delay-lines.md) and one of the biggest ideas in the string model.

## Radians per sample

Filter maths uses **normalized angular frequency**:

```math
\omega = 2\pi \frac{f}{f_s} \quad \text{(radians per sample)}
```

0 is DC (0 Hz) and $\pi$ is Nyquist. Whenever the code says `w0` it means the string's fundamental in these units:

```cpp
const double w0 = 2.0 * kPi * frequency / sampleRate;
```

## MIDI notes, Hz and cents

MIDI numbers notes in semitones: 60 is middle C and 69 is A4 at 440 Hz. In 12-tone equal temperament:

```math
f = 440 \cdot 2^{(n - 69)/12}
```

That is `Tuning::getStringFrequency`.

A **cent** is 1/100 of a semitone. The difference between two frequencies in cents is $1200 \log_2(f_1/f_2)$. People
start to notice tuning errors at around 3 to 5 cents, which is why the tests require every string to be within
**±3 cents** (see [Testing DSP](10-testing-dsp.md)).

## Decibels

Loudness is perceived roughly logarithmically, so amplitude ratios are expressed in decibels:

```math
\text{dB} = 20 \log_{10}(a)
```

| Amplitude | dB |
|---|---|
| 1.0 | 0 dB |
| 0.5 | −6 dB |
| 0.1 | −20 dB |
| 0.001 | −60 dB |
| 0.00001 | −100 dB |

**−60 dB** defines **T60**, the time for a sound to decay to one thousandth of its amplitude. Zither's Decay knob is a
T60. **−100 dB** is the threshold where a string goes to sleep. JUCE converts with `juce::Decibels::decibelsToGain`,
which is used for the Output knob.

## Sample-rate independence

A rule of this codebase: **specify everything in Hz or seconds, and convert to samples in `prepare`.**

If you wrote "lowpass coefficient = 0.1", the cutoff frequency would change with the sample rate. Instead, compute the
coefficient from a cutoff in Hz:

```cpp
// FxChain::OnePole: cutoff in Hz → per-sample coefficient
coef = 1.0 - std::exp (-2π * hz / sampleRate);
```

The same idea applies to times: the strum interval is in ms and is converted to samples with `intervalMs * 0.001 * sampleRate`.

## Velocity and loudness curves

MIDI velocity (0 to 127, which JUCE gives as 0 to 1) isn't used linearly. In the exciter:

```cpp
double amp = 0.05 + 0.95 * std::pow (vel, 1.5);
```

The 1.5 exponent makes soft playing quieter relative to loud playing, which feels more natural, and the 0.05 floor
keeps the softest touch audible.

## Stereo panning

Each string has a fixed position between left and right: bass slightly left, treble slightly right. **Constant-power
panning** keeps the perceived loudness steady across positions:

```math
g_L = \cos\theta, \quad g_R = \sin\theta, \quad g_L^2 + g_R^2 = 1
```

That's in `StringBank::setTuning`. A naive linear pan ($g_L = 1 - x$, $g_R = x$) dips by 3 dB in the middle.

## Try it

- Work out the period in samples of your favourite note at 44.1 kHz and at 96 kHz.
- How many cents off would the top string (4186 Hz) be at 44.1 kHz if its period were rounded to a whole number?
  (Answer on the [Delay lines](04-delay-lines.md) page.)
