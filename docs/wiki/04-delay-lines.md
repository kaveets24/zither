# 4. Delay lines

A **delay line** remembers the last $N$ samples and lets you read back the one from $D$ samples ago. It is the
building block of strings, echoes, choruses and reverbs.

## The circular buffer

Instead of shifting every sample along each time (too slow), write into an array at a moving **write position**
and wrap around at the end:

```text
buffer:  [ s5 | s6 | s7 | s0 | s1 | s2 | s3 | s4 ]
                       ^ writePos (next write)
read D samples ago  =  buffer[(writePos - D) wrapped]
```

`WaveguideString` makes the buffer a **power of two** in size, so wrapping is a single bitwise AND instead of a
division or a branch:

```cpp
// prepare(): round up to a power of two, sized for the lowest possible string (30 Hz)
while (size < maxDelay) size <<= 1;
mask = size - 1;

// processSample():
const float x = buffer[(writePos - delayInt) & mask];
...
writePos = (writePos + 1) & mask;
```

It's allocated once, in `prepare`, for the **longest** delay ever needed. Changing a string's pitch later only
changes `delayInt`, and never allocates (see [Realtime safety](09-realtime-safety.md)).

## Why fractional delay matters

A string's pitch is set by its loop delay: $P = f_s / f_0$ samples. If you could only delay by whole samples, high
notes would be badly out of tune. Take the top string, C8 (4186 Hz), at 44.1 kHz:

```math
P = 44100 / 4186 = 10.535 \text{ samples}
```

| Delay used | Pitch | Error |
|---|---|---|
| 10 samples | 4410 Hz | **+90 cents** (almost a semitone sharp) |
| 11 samples | 4009 Hz | **−75 cents** |
| 10.535 samples | 4186 Hz | 0 |

So we need to delay by **fractions of a sample**: read "between" two samples.

## Linear interpolation

The simplest approach: blend the two neighbouring samples.

```math
y = (1 - \alpha)\,x[n - k] + \alpha\,x[n - k - 1]
```

It's cheap, and JUCE's `dsp::DelayLine<..., Linear>` uses it for the ping-pong delay. But it's secretly a
**lowpass filter** whose strength depends on $\alpha$: at $\alpha = 0.5$ it's an averaging filter that noticeably dulls
high frequencies. In an echo that's harmless. **Inside a string loop it's a problem:** the signal passes through it
thousands of times, so strings would ring brighter or duller depending on how their period happens to round.

## Thiran allpass interpolation

The fix is an **allpass** fractional delay (see [Filters](03-filters.md)). It has magnitude 1 at every frequency, so
it never dulls the sound; it only delays. For a desired delay $d$ (kept in the range 0.5 to 1.5 samples, where it
behaves best):

```math
\eta = \frac{1 - d}{1 + d}, \qquad H(z) = \frac{\eta + z^{-1}}{1 + \eta\,z^{-1}}
```

This is the first-order **Thiran** allpass, in `WaveguideString::updateCoefficients`:

```cpp
// What's left goes to the integer delay plus a Thiran allpass for the fraction (d in [0.5, 1.5)).
const double remaining = period - tauLoss - tauDisp;
const int length = static_cast<int> (std::floor (remaining - 0.5));
const double target = remaining - length;
```

**The refinement loop:** the formula gives exactly $d$ samples of delay at DC, but slightly less at higher
frequencies, and the string's fundamental can be thousands of Hz. So the code measures the actual delay at $f_0$ and
nudges $d$ a few times until it's exact **at the fundamental**:

```cpp
for (int i = 0; i < 4; ++i)
{
    const double eta = (1.0 - d) / (1.0 + d);
    d = d + target - allpassPhaseDelay (eta, w0);   // correct by the error
}
```

The trade-off: an allpass interpolator smears quickly changing delays, so it suits a string with a fixed pitch,
not a delay time that is swept continuously.

## Modulated delay = pitch bend

If a delay's read position moves while sound is playing, the pitch of what's coming out shifts, just as a tape slows
down. The ping-pong delay smooths changes to its time over 0.25 s (`delaySamples.reset (sampleRate, 0.25)`), so
turning the Delay Time knob gives a gentle, tape-like bend instead of clicks and zipper noise.

## Feedback

Feed a delay's output back into its input with gain $g$ and you get repeating echoes. Each loop multiplies the echo
by $g$, so **$|g| < 1$ is required for stability.** The string is a feedback delay with $g$ extremely close to 1
(e.g. 0.997), which is why it rings for so long. The ping-pong delay uses $g$ up to 0.95.

## Try it

- In `WaveguideString::prepare`, how big does the buffer get at 96 kHz for a 30 Hz lowest string? (96000/30 + 8 ≈
  3208, rounded up to a power of two.)
- Temporarily set `thiranCoef` to 0 (which turns the Thiran filter into a plain one-sample delay) and run the pitch
  test. Which strings fail, and by how much?
