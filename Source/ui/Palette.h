#pragma once

#include <juce_graphics/juce_graphics.h>

namespace zither::ui::palette
{
inline const juce::Colour abyss     { 0xff040f1c };
inline const juce::Colour deepWater { 0xff0a2a3d };
inline const juce::Colour lagoon    { 0xff12485a };
inline const juce::Colour aqua      { 0xff7fe7d9 };
inline const juce::Colour pearl     { 0xffe8f4f0 };
inline const juce::Colour coral     { 0xffe08aa6 };
inline const juce::Colour violet    { 0xff6b4fa8 };
inline const juce::Colour melt      { 0xff2a1f5c };
inline const juce::Colour mist      { 0xff9fb8c4 };

/** Lowercase, letter-spaced UI type. */
inline juce::Font font (float height, float kerning = 0.12f)
{
    return juce::Font (juce::FontOptions (height)).withExtraKerningFactor (kerning);
}
} // namespace zither::ui::palette
