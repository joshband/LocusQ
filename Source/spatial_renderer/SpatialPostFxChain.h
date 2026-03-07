#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace locusq::spatial_post_fx_chain
{

template <typename EarlyReflectionsType, typename ReverbType>
inline void applyRoomFxIfEnabled (
    bool roomEnabled,
    bool earlyReflectionsOnly,
    EarlyReflectionsType& earlyReflections,
    ReverbType& fdnReverb,
    juce::AudioBuffer<float>& accumBuffer)
{
    if (! roomEnabled)
        return;

    earlyReflections.process (accumBuffer);
    if (! earlyReflectionsOnly)
        fdnReverb.process (accumBuffer);
}

template <typename DelayBuffer>
inline void processSpeakerDelayLine (
    float* channelData,
    int numSamples,
    int delaySamples,
    DelayBuffer& delayLine,
    int& delayWritePos,
    int maxDelaySamples) noexcept
{
    if (delaySamples <= 0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        delayLine[static_cast<std::size_t> (delayWritePos)] = channelData[i];

        int readPos = delayWritePos - delaySamples;
        if (readPos < 0)
            readPos += maxDelaySamples;

        channelData[i] = delayLine[static_cast<std::size_t> (readPos)];
        delayWritePos = (delayWritePos + 1) % maxDelaySamples;
    }
}

inline void applySpeakerTrim (
    float* channelData,
    int numSamples,
    juce::SmoothedValue<float>& smoothedTrim) noexcept
{
    for (int i = 0; i < numSamples; ++i)
        channelData[i] *= smoothedTrim.getNextValue();
}

} // namespace locusq::spatial_post_fx_chain
