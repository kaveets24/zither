# 7. Tuning and strumming

This part is less signal processing and more musical logic and timing, but the timing is where audio code often
goes wrong. Code: [`Source/tuning/Tuning.cpp`](../../Source/tuning/Tuning.cpp) and
[`Source/tuning/Strummer.h`](../../Source/tuning/Strummer.h).

## An open-tuned instrument

A piano has every chromatic note. A zither or harp is **tuned to one scale**: the strings are the only notes there
are. Zither builds its strings from a **root**, a **mode** and a **lowest octave**.

A mode is a set of semitone steps above the root:

| Mode | Degrees (semitones above root) |
|---|---|
| Lydian (default) | 0 2 4 6 7 9 11 |
| Ionian (major) | 0 2 4 5 7 9 11 |
| Mixolydian | 0 2 4 5 7 9 10 |
| Dorian | 0 2 3 5 7 9 10 |
| Aeolian (natural minor) | 0 2 3 5 7 8 10 |
| Major pentatonic | 0 2 4 7 9 |
| Minor pentatonic | 0 3 5 7 10 |
| Major triad | 0 4 7 |
| Suspended | 0 2 7 |

Strings walk up the degrees, repeating each octave:

```cpp
note = base + 12 * (i / modeSize) + degrees[i % modeSize];
```

**Example (C Lydian, lowest octave 2):** C2 D2 E2 F♯2 G2 A2 B2 C3 D3 … which is MIDI 36 38 40 42 43 45 47 48 50 …
Strings above MIDI 108 are dropped.

Lydian is the default because its raised 4th (F♯ in C) has no "avoid note" against the root: any combination of
strings sounds consonant and floating, which suits the Laraaji-style wash this instrument is built around.

## No wrong notes: snapping keys to strings

A MIDI keyboard has 12 keys per octave; the tuning might have 3, 5 or 7. `Tuning::nearestString` maps every key to
a string:

1. **Fold by octave:** keys below the lowest string move up by octaves, and keys above the highest move down.
2. **Snap to the nearest string.** On a tie, the lower string wins.

So in C Lydian, pressing F (MIDI 65) is exactly between E4 (64) and F♯4 (66) and plays E4. Any key you press plays
something in the scale. This is core value #1: **no wrong notes**.

## The strum

Each key press can sweep across several neighbouring strings, like a hand brushed across a harp:

```text
key → string 10 now
         string 11 at +40 ms (velocity × 0.86)
         string 12 at +80 ms (velocity × 0.86²)
         string 13 at +120 ms (velocity × 0.86³)
```

- **Span** (1 to 8): how many strings, including the played one.
- **Direction:** up, down or alternating (each strum reverses).
- **Strum Speed:** the interval between strings, in ms.
- **Humanize:** random jitter of up to ±35% of an interval in timing and ±20% in velocity, so it never sounds
  machine-gunned.

The played string sounds immediately. The rest are put on a schedule with their due time in **samples**:

```cpp
const double position = i + humanize * 0.35 * nextBipolarRandom (rng);
const auto offset = max (1, llround (position * interval));
schedule (now + offset, index, velocity);
```

The queue is a **fixed-size array** (256 entries) with swap-remove, so it never allocates on the audio thread (see
[Realtime safety](09-realtime-safety.md)).

## Sample-accurate timing

`processBlock` gets a whole block at once, say 512 samples (10.7 ms at 48 kHz). If every strike happened at the
start of the block, notes would land up to 10 ms early or late at random. That's enough to hear as sloppiness or a
flam, and it would change with the buffer size.

So `processBlock` renders the block **in pieces**, splitting at every MIDI event and every scheduled strike:

```text
block:  |------------------------------------------------|
events:      ^note-on          ^strum strike      ^strum strike
render: |----|-----------------|------------------|-------|
```

```cpp
while (pos < numSamples)
{
    handle MIDI events at pos;
    strummer.popDue (excite);                       // fire strikes due now
    next = min (next MIDI event, next strum strike, end of block);
    strings.render (left + pos, right + pos, next - pos);
    strummer.advance (next - pos);
    pos = next;
}
```

Strums can also cross block boundaries, because the strummer keeps its own running sample clock (`now`), not a
position within a block.

## Try it

- Set Strum Span to 8 and Strum Speed to 150 ms, then hold a single key. Now set Humanize to 0% and listen to it
  turn mechanical.
- Switch to Major Triad mode and play random keys. Everything should land on C, E or G.
- Read the strum tests in [`tests/TuningTests.cpp`](../../tests/TuningTests.cpp). They check the exact sample times
  of a strum.
