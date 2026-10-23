#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

namespace
{
float softClip (float x) noexcept
{
    constexpr float knee = 0.85f;
    const float magnitude = std::abs (x);
    if (magnitude <= knee)
        return x;

    return std::copysign (knee + (1.0f - knee) * std::tanh ((magnitude - knee) / (1.0f - knee)), x);
}
} // namespace

ZitherAudioProcessor::ZitherAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "ZitherState", zither::param::createLayout())
{
    noteToString.fill (-1);
    parameters.state.setProperty (stateVersionProperty, kStateVersion, nullptr);
    parameters.state.setProperty (visualsProperty, true, nullptr);
}

void ZitherAudioProcessor::strikeString (int stringIndex, float velocity)
{
    const auto scope = uiStrikeFifo.write (1);
    if (scope.blockSize1 > 0)
        uiStrikes[static_cast<size_t> (scope.startIndex1)] = { stringIndex, velocity };
    else if (scope.blockSize2 > 0)
        uiStrikes[static_cast<size_t> (scope.startIndex2)] = { stringIndex, velocity };
}

void ZitherAudioProcessor::publishStringActivity() noexcept
{
    for (int i = 0; i < zither::Tuning::kMaxStrings; ++i)
    {
        auto& level = stringLevels[static_cast<size_t> (i)];
        if (const float peak = strings.takePeak (i); peak > level.load (std::memory_order_relaxed))
            level.store (peak, std::memory_order_relaxed);

        strikeCounts[static_cast<size_t> (i)].store (strings.getStrikeCount (i), std::memory_order_relaxed);
    }
}

bool ZitherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ZitherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    strings.prepare (sampleRate);
    strummer.prepare (sampleRate);
    fx.prepare ({ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 });

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getValue (zither::param::output)));

    tuningValid = false;
    noteToString.fill (-1);
    pendingDamp.fill (false);
    sustainHeld = false;

    updateParameters();
}

void ZitherAudioProcessor::updateParameters()
{
    using namespace zither;

    const TuningSettings tuningSettings { getIndex (param::root),
                                          static_cast<Mode> (getIndex (param::mode)),
                                          getIndex (param::numStrings),
                                          getIndex (param::lowestOctave) };
    if (! tuningValid || tuningSettings != currentTuning)
    {
        currentTuning = tuningSettings;
        tuning.set (currentTuning);
        strings.setTuning (tuning);
        tuningValid = true;
    }

    releaseDamping = getValue (param::damping);
    strings.setStringParams ({ getValue (param::decay), getValue (param::brightness), getValue (param::inharmonicity), releaseDamping });
    strings.setExciterSettings ({ static_cast<ExciterType> (getIndex (param::exciter)), getValue (param::hardness), getValue (param::strikePos) });

    strummer.setSettings ({ getValue (param::strum) > 0.5f,
                            getValue (param::strumSpeed),
                            getIndex (param::strumSpan),
                            static_cast<StrumDirection> (getIndex (param::strumDir)),
                            getValue (param::humanize) });

    fx.setSettings ({ getValue (param::phaserMix), getValue (param::phaserRate),
                      getValue (param::delayTime), getValue (param::delayFeedback), getValue (param::delayMix),
                      getValue (param::reverbSize), getValue (param::reverbMix) });

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (getValue (param::output)));
}

void ZitherAudioProcessor::handleMidi (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        const int note = message.getNoteNumber();
        const int stringIndex = tuning.nearestString (note);
        if (stringIndex < 0)
            return;

        noteToString[static_cast<size_t> (note)] = stringIndex;
        pendingDamp[static_cast<size_t> (stringIndex)] = false;
        strummer.noteOn (stringIndex, message.getFloatVelocity(), tuning.getNumStrings(),
                         [this] (int s, float v) { strings.excite (s, v); });
    }
    else if (message.isNoteOff())
    {
        // Zither strings ring after release unless release damping is turned up.
        auto& mapped = noteToString[static_cast<size_t> (message.getNoteNumber())];
        const int stringIndex = std::exchange (mapped, -1);
        if (stringIndex < 0 || releaseDamping <= 0.0f)
            return;

        if (sustainHeld)
            pendingDamp[static_cast<size_t> (stringIndex)] = true;
        else
            strings.setDamped (stringIndex, true);
    }
    else if (message.isSustainPedalOn())
    {
        sustainHeld = true;
    }
    else if (message.isSustainPedalOff())
    {
        sustainHeld = false;
        for (size_t i = 0; i < pendingDamp.size(); ++i)
            if (std::exchange (pendingDamp[i], false))
                strings.setDamped (static_cast<int> (i), true);
    }
    else if (message.isAllSoundOff())
    {
        strings.reset();
        strummer.clear();
    }
    else if (message.isAllNotesOff())
    {
        strummer.clear();
    }
}

void ZitherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    updateParameters();

    {
        const auto scope = uiStrikeFifo.read (uiStrikeFifo.getNumReady());
        scope.forEach ([this] (int index) {
            const auto& s = uiStrikes[static_cast<size_t> (index)];
            strings.excite (s.stringIndex, s.velocity);
        });
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    auto excite = [this] (int s, float v) { strings.excite (s, v); };

    // Render in sub-blocks split at every MIDI event and queued strum strike.
    auto event = midi.cbegin();
    int pos = 0;
    while (pos < numSamples)
    {
        for (; event != midi.cend() && (*event).samplePosition <= pos; ++event)
            handleMidi ((*event).getMessage());

        strummer.popDue (excite);

        int next = numSamples;
        if (event != midi.cend())
            next = std::min (next, (*event).samplePosition);
        if (const int untilStrike = strummer.samplesUntilNext(); untilStrike >= 0)
            next = std::min (next, pos + untilStrike);
        next = std::max (next, pos + 1);

        strings.render (left + pos, right + pos, next - pos);
        strummer.advance (next - pos);
        pos = next;
    }

    for (; event != midi.cend(); ++event)
        handleMidi ((*event).getMessage());

    publishStringActivity();
    fx.process (buffer);

    for (int i = 0; i < numSamples; ++i)
    {
        const float gain = outputGain.getNextValue();
        left[i] = softClip (left[i] * gain);
        right[i] = softClip (right[i] * gain);
    }
}

juce::AudioProcessorEditor* ZitherAudioProcessor::createEditor()
{
    return new ZitherAudioProcessorEditor (*this);
}

void ZitherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ZitherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    auto loaded = juce::ValueTree::fromXml (*xml);

    // Nothing to migrate yet: every saved version so far shares one layout. The APVTS ignores unknown
    // properties and parameters (from a newer build) and loads parameters the session lacks at their
    // defaults. tests/PluginStateTests.cpp pins both, so a JUCE upgrade can't quietly change them.
    loaded.setProperty (stateVersionProperty, kStateVersion, nullptr);
    parameters.replaceState (loaded);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ZitherAudioProcessor();
}
