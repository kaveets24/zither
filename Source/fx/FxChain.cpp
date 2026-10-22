#include "FxChain.h"

namespace zither
{
void FxChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    phaser.prepare (spec);
    phaser.setDepth (0.7f);
    phaser.setCentreFrequency (900.0f);
    phaser.setFeedback (0.35f);

    maxDelaySamples = static_cast<float> (2.5 * sampleRate);
    delay.setMaximumDelayInSamples (static_cast<int> (maxDelaySamples) + 4);
    delay.prepare ({ spec.sampleRate, spec.maximumBlockSize, 2 });

    // A slow glide on delay time gives tape-like pitch bends instead of zipper noise.
    delaySamples.reset (sampleRate, 0.25);
    feedback.reset (sampleRate, 0.05);
    delayMix.reset (sampleRate, 0.05);

    for (auto& f : toneLowpass)
        f.setCutoff (4500.0, sampleRate);
    for (auto& f : toneHighpass)
        f.setCutoff (140.0, sampleRate);

    reverb.prepare (spec);
    reverbParams.damping = 0.35f;
    reverbParams.width = 1.0f;

    setSettings ({});
    delaySamples.setCurrentAndTargetValue (delaySamples.getTargetValue());
    reset();
}

void FxChain::reset()
{
    phaser.reset();
    delay.reset();
    for (auto& f : toneLowpass)
        f.state = 0.0f;
    for (auto& f : toneHighpass)
        f.state = 0.0f;
    reverb.reset();
}

void FxChain::setSettings (const FxSettings& s)
{
    phaser.setMix (s.phaserMix);
    phaser.setRate (s.phaserRateHz);

    delaySamples.setTargetValue (juce::jlimit (1.0f, maxDelaySamples, s.delayTimeMs * 0.001f * static_cast<float> (sampleRate)));
    feedback.setTargetValue (s.delayFeedback);
    delayMix.setTargetValue (s.delayMix);

    // juce::dsp::Reverb (Freeverb) scales wet by 3 and dry by 2 internally.
    auto params = reverbParams;
    params.roomSize = 0.3f + 0.68f * s.reverbSize;
    params.wetLevel = 0.45f * s.reverbMix;
    params.dryLevel = 0.5f * (1.0f - 0.35f * s.reverbMix);

    if (! juce::approximatelyEqual (params.roomSize, reverbParams.roomSize)
        || ! juce::approximatelyEqual (params.wetLevel, reverbParams.wetLevel)
        || ! juce::approximatelyEqual (params.dryLevel, reverbParams.dryLevel))
    {
        reverbParams = params;
        reverb.setParameters (reverbParams);
    }
}

float FxChain::feedbackTone (float x, size_t channel) noexcept
{
    const float lp = toneLowpass[channel].process (x);
    return lp - toneHighpass[channel].process (lp);
}

void FxChain::processDelay (float* left, float* right, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        const float d = delaySamples.getNextValue();
        const float fb = feedback.getNextValue();
        const float mix = delayMix.getNextValue();

        const float echoL = delay.popSample (0, d);
        const float echoR = delay.popSample (1, d);

        // Ping-pong: input enters on the left, and each repeat crosses to the other side.
        const float input = 0.5f * (left[i] + right[i]);
        delay.pushSample (0, input + fb * feedbackTone (echoR, 0));
        delay.pushSample (1, fb * feedbackTone (echoL, 1));

        left[i] += mix * echoL;
        right[i] += mix * echoR;
    }
}

void FxChain::process (juce::AudioBuffer<float>& buffer) noexcept
{
    jassert (buffer.getNumChannels() >= 2);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    phaser.process (context);
    processDelay (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
    reverb.process (context);
}
} // namespace zither
