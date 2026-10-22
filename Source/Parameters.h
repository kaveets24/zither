#pragma once

#include "tuning/Tuning.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace zither::param
{
// Tuning
inline constexpr auto root          = "root";
inline constexpr auto mode          = "mode";
inline constexpr auto numStrings    = "numStrings";
inline constexpr auto lowestOctave  = "lowestOctave";

// Strike
inline constexpr auto exciter       = "exciter";
inline constexpr auto hardness      = "hardness";
inline constexpr auto strikePos     = "strikePos";

// String
inline constexpr auto decay         = "decay";
inline constexpr auto brightness    = "brightness";
inline constexpr auto inharmonicity = "inharmonicity";
inline constexpr auto damping       = "damping";

// Strum
inline constexpr auto strum         = "strum";
inline constexpr auto strumSpeed    = "strumSpeed";
inline constexpr auto strumSpan     = "strumSpan";
inline constexpr auto strumDir      = "strumDir";
inline constexpr auto humanize      = "humanize";

// FX
inline constexpr auto phaserMix     = "phaserMix";
inline constexpr auto phaserRate    = "phaserRate";
inline constexpr auto delayTime     = "delayTime";
inline constexpr auto delayFeedback = "delayFeedback";
inline constexpr auto delayMix      = "delayMix";
inline constexpr auto reverbSize    = "reverbSize";
inline constexpr auto reverbMix     = "reverbMix";
inline constexpr auto output        = "output";

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;

    auto id = [] (const char* name) { return ParameterID { name, 1 }; };
    auto unit = [] (const char* label) { return AudioParameterFloatAttributes().withLabel (label); };
    auto skewed = [] (float lo, float hi, float centre) {
        NormalisableRange<float> range { lo, hi };
        range.setSkewForCentre (centre);
        return range;
    };

    StringArray modeNames;
    for (int m = 0; m < static_cast<int> (Mode::numModes); ++m)
        modeNames.add (getModeName (static_cast<Mode> (m)));

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (id (root), "Root",
                    StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0),
                std::make_unique<AudioParameterChoice> (id (mode), "Mode", modeNames, 0),
                std::make_unique<AudioParameterInt> (id (numStrings), "Strings", 12, Tuning::kMaxStrings, 30),
                std::make_unique<AudioParameterInt> (id (lowestOctave), "Lowest Octave", 1, 4, 2));

    layout.add (std::make_unique<AudioParameterChoice> (id (exciter), "Strike", StringArray { "Hammer", "Pluck" }, 0),
                std::make_unique<AudioParameterFloat> (id (hardness), "Hardness", NormalisableRange<float> { 0.0f, 1.0f }, 0.35f),
                std::make_unique<AudioParameterFloat> (id (strikePos), "Strike Position", NormalisableRange<float> { 0.02f, 0.5f }, 0.13f));

    layout.add (std::make_unique<AudioParameterFloat> (id (decay), "Decay", skewed (1.0f, 40.0f, 10.0f), 18.0f, unit ("s")),
                std::make_unique<AudioParameterFloat> (id (brightness), "Brightness", NormalisableRange<float> { 0.0f, 1.0f }, 0.6f),
                std::make_unique<AudioParameterFloat> (id (inharmonicity), "Inharmonicity", NormalisableRange<float> { 0.0f, 1.0f }, 0.2f),
                std::make_unique<AudioParameterFloat> (id (damping), "Release Damping", NormalisableRange<float> { 0.0f, 1.0f }, 0.0f));

    layout.add (std::make_unique<AudioParameterBool> (id (strum), "Strum", true),
                std::make_unique<AudioParameterFloat> (id (strumSpeed), "Strum Speed", skewed (5.0f, 150.0f, 40.0f), 40.0f, unit ("ms")),
                std::make_unique<AudioParameterInt> (id (strumSpan), "Strum Span", 1, 8, 4),
                std::make_unique<AudioParameterChoice> (id (strumDir), "Strum Direction", StringArray { "Up", "Down", "Alternate" }, 0),
                std::make_unique<AudioParameterFloat> (id (humanize), "Humanize", NormalisableRange<float> { 0.0f, 1.0f }, 0.35f));

    layout.add (std::make_unique<AudioParameterFloat> (id (phaserMix), "Phaser Mix", NormalisableRange<float> { 0.0f, 1.0f }, 0.25f),
                std::make_unique<AudioParameterFloat> (id (phaserRate), "Phaser Rate", skewed (0.02f, 2.0f, 0.25f), 0.12f, unit ("Hz")),
                std::make_unique<AudioParameterFloat> (id (delayTime), "Delay Time", skewed (50.0f, 2000.0f, 500.0f), 640.0f, unit ("ms")),
                std::make_unique<AudioParameterFloat> (id (delayFeedback), "Delay Feedback", NormalisableRange<float> { 0.0f, 0.95f }, 0.55f),
                std::make_unique<AudioParameterFloat> (id (delayMix), "Delay Mix", NormalisableRange<float> { 0.0f, 1.0f }, 0.3f),
                std::make_unique<AudioParameterFloat> (id (reverbSize), "Reverb Size", NormalisableRange<float> { 0.0f, 1.0f }, 0.85f),
                std::make_unique<AudioParameterFloat> (id (reverbMix), "Reverb Mix", NormalisableRange<float> { 0.0f, 1.0f }, 0.45f),
                std::make_unique<AudioParameterFloat> (id (output), "Output", NormalisableRange<float> { -36.0f, 6.0f }, -6.0f, unit ("dB")));

    return layout;
}
} // namespace zither::param
