#pragma once

#include <juce_dsp/juce_dsp.h>

namespace zither
{
struct FxSettings
{
    float phaserMix = 0.25f;
    float phaserRateHz = 0.12f;
    float delayTimeMs = 640.0f;
    float delayFeedback = 0.55f;
    float delayMix = 0.3f;
    float reverbSize = 0.85f;
    float reverbMix = 0.45f;
};

/**
    The "electric zither" tail: slow phaser -> filtered ping-pong delay -> large reverb.
    Each echo gets darker and thinner, so long feedback settings blur into a wash instead of building up.
    A shimmer (pitch-shifted) reverb is planned for phase 2.
*/
class FxChain
{
public:
    void prepare (const juce::dsp::ProcessSpec&);
    void reset();
    void setSettings (const FxSettings&);
    void process (juce::AudioBuffer<float>&) noexcept;

private:
    struct OnePole
    {
        float coef = 0.0f, state = 0.0f;
        void setCutoff (double hz, double rate) { coef = static_cast<float> (1.0 - std::exp (-juce::MathConstants<double>::twoPi * hz / rate)); }
        float process (float x) noexcept { return state += coef * (x - state); }
    };

    float feedbackTone (float x, size_t channel) noexcept;
    void processDelay (float* left, float* right, int numSamples) noexcept;

    double sampleRate = 44100.0;
    float maxDelaySamples = 0.0f;

    juce::dsp::Phaser<float> phaser;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
    juce::SmoothedValue<float> delaySamples, feedback, delayMix;
    std::array<OnePole, 2> toneLowpass, toneHighpass;

    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
};
} // namespace zither
