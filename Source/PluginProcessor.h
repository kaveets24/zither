#pragma once

#include "dsp/StringBank.h"
#include "fx/FxChain.h"
#include "tuning/Strummer.h"
#include "tuning/Tuning.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

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
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

private:
    void updateParameters();
    void handleMidi (const juce::MidiMessage&);
    float getValue (const char* id) const { return parameters.getRawParameterValue (id)->load(); }
    int getIndex (const char* id) const { return static_cast<int> (getValue (id)); }

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZitherAudioProcessor)
};
