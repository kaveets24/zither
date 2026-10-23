#include "Parameters.h"
#include "PluginProcessor.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// Saved sessions are a contract with users: a session saved by any earlier build must load in this one.

using Catch::Matchers::WithinAbs;
namespace param = zither::param;

namespace
{
juce::RangedAudioParameter& parameter (ZitherAudioProcessor& p, const char* id)
{
    auto* ranged = p.getParameters().getParameter (id);
    REQUIRE (ranged != nullptr);
    return *ranged;
}

void setValue (ZitherAudioProcessor& p, const char* id, float value)
{
    auto& ranged = parameter (p, id);
    ranged.setValueNotifyingHost (ranged.convertTo0to1 (value));
}

float getValue (ZitherAudioProcessor& p, const char* id)
{
    return p.getParameters().getRawParameterValue (id)->load();
}

float getDefault (ZitherAudioProcessor& p, const char* id)
{
    auto& ranged = parameter (p, id);
    return ranged.convertFrom0to1 (ranged.getDefaultValue());
}

std::unique_ptr<juce::XmlElement> save (ZitherAudioProcessor& p)
{
    juce::MemoryBlock block;
    p.getStateInformation (block);
    auto xml = juce::AudioProcessor::getXmlFromBinary (block.getData(), static_cast<int> (block.getSize()));
    REQUIRE (xml != nullptr);
    return xml;
}

void load (ZitherAudioProcessor& p, const juce::XmlElement& xml)
{
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary (xml, block);
    p.setStateInformation (block.getData(), static_cast<int> (block.getSize()));
}
} // namespace

TEST_CASE ("A saved session restores parameters and editor state", "[state]")
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    ZitherAudioProcessor original;
    setValue (original, param::decay, 30.0f);
    setValue (original, param::mode, 3.0f);
    setValue (original, param::strumSpan, 7.0f);
    auto& state = original.getParameters().state;
    state.setProperty (ZitherAudioProcessor::visualsProperty, false, nullptr);
    state.setProperty (ZitherAudioProcessor::editorWidthProperty, 1200, nullptr);
    state.setProperty (ZitherAudioProcessor::editorHeightProperty, 800, nullptr);

    const auto saved = save (original);
    REQUIRE (saved->getIntAttribute ("stateVersion") >= 1);

    ZitherAudioProcessor restored;
    load (restored, *saved);

    REQUIRE_THAT (getValue (restored, param::decay), WithinAbs (30.0, 0.01));
    REQUIRE (static_cast<int> (getValue (restored, param::mode)) == 3);
    REQUIRE (static_cast<int> (getValue (restored, param::strumSpan)) == 7);

    const auto& restoredState = restored.getParameters().state;
    REQUIRE_FALSE (static_cast<bool> (restoredState.getProperty (ZitherAudioProcessor::visualsProperty)));
    REQUIRE (static_cast<int> (restoredState.getProperty (ZitherAudioProcessor::editorWidthProperty)) == 1200);
    REQUIRE (static_cast<int> (restoredState.getProperty (ZitherAudioProcessor::editorHeightProperty)) == 800);
}

TEST_CASE ("An unversioned session missing newer parameters loads them at their defaults", "[state]")
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Simulate a session from an older build: no version number, and no decay parameter yet.
    ZitherAudioProcessor oldBuild;
    setValue (oldBuild, param::reverbMix, 0.9f);
    auto oldSession = save (oldBuild);
    oldSession->removeAttribute ("stateVersion");
    auto* decay = oldSession->getChildByAttribute ("id", param::decay);
    REQUIRE (decay != nullptr);
    oldSession->removeChildElement (decay, true);

    // The instance it loads into has been tweaked, which must not leak into the loaded session,
    // or the same session could sound different on each load.
    ZitherAudioProcessor current;
    setValue (current, param::decay, 5.0f);
    REQUIRE_THAT (getValue (current, param::decay), WithinAbs (5.0, 0.01));
    load (current, *oldSession);

    REQUIRE_THAT (getValue (current, param::decay), WithinAbs (getDefault (current, param::decay), 0.01));
    REQUIRE_THAT (getValue (current, param::reverbMix), WithinAbs (0.9, 0.001));
    REQUIRE (save (current)->getIntAttribute ("stateVersion") >= 1);
}

TEST_CASE ("Unknown parameters and corrupt data are ignored", "[state]")
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    ZitherAudioProcessor processor;
    setValue (processor, param::delayMix, 0.7f);

    // A session from a newer build may carry parameters this build doesn't know.
    auto newerSession = save (processor);
    auto* unknown = newerSession->createNewChildElement ("PARAM");
    unknown->setAttribute ("id", "fromTheFuture");
    unknown->setAttribute ("value", 0.5);
    load (processor, *newerSession);
    REQUIRE_THAT (getValue (processor, param::delayMix), WithinAbs (0.7, 0.001));

    const char garbage[] = "not a zither session";
    processor.setStateInformation (garbage, static_cast<int> (sizeof (garbage)));
    processor.setStateInformation (nullptr, 0);
    REQUIRE_THAT (getValue (processor, param::delayMix), WithinAbs (0.7, 0.001));
}
