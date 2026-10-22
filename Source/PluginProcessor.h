#pragma once

#include "dsp/StringBank.h"
#include "fx/FxChain.h"
#include "tuning/Strummer.h"
#include "tuning/Tuning.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

class ZitherAudioProcessor final : public juce::AudioProcessor
{
public:
    ZitherAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    // UI <-> audio, lock-free.
    /** Message thread only: strikes a string directly (the on-screen strings bypass the strummer). */
    void strikeString (int stringIndex, float velocity);
    /** Peak level of a string since the UI last asked. */
    float takeStringLevel (int stringIndex) noexcept { return stringLevels[static_cast<size_t> (stringIndex)].exchange (0.0f, std::memory_order_relaxed); }
    uint32_t getStrikeCount (int stringIndex) const noexcept { return strikeCounts[static_cast<size_t> (stringIndex)].load (std::memory_order_relaxed); }

    static constexpr auto visualsProperty = "visuals";

private:
    void updateParameters();
    void handleMidi (const juce::MidiMessage&);
    void publishStringActivity() noexcept;
    float getValue (const char* id) const { return parameters.getRawParameterValue (id)->load(); }
    int getIndex (const char* id) const { return static_cast<int> (getValue (id)); }

    juce::AudioProcessorValueTreeState parameters;

    zither::Tuning tuning;
    zither::TuningSettings currentTuning;
    bool tuningValid = false;

    zither::StringBank strings;
    zither::Strummer strummer;
    zither::FxChain fx;
    juce::SmoothedValue<float> outputGain;

    std::array<int, 128> noteToString {};
    std::array<bool, zither::Tuning::kMaxStrings> pendingDamp {};
    bool sustainHeld = false;
    float releaseDamping = 0.0f;

    struct UiStrike
    {
        int stringIndex;
        float velocity;
    };
    static constexpr int kUiStrikeCapacity = 256;
    juce::AbstractFifo uiStrikeFifo { kUiStrikeCapacity };
    std::array<UiStrike, kUiStrikeCapacity> uiStrikes {};

    std::array<std::atomic<float>, zither::Tuning::kMaxStrings> stringLevels {};
    std::array<std::atomic<uint32_t>, zither::Tuning::kMaxStrings> strikeCounts {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZitherAudioProcessor)
};
