#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../SpatialRenderer.h"

#include <array>
#include <cmath>

namespace locusq::processor_core
{
inline int readSnapshotOutputChannels (int mainBusOutputChannels, int totalOutputChannels) noexcept
{
    if (mainBusOutputChannels > 0)
        return mainBusOutputChannels;

    return juce::jmax (1, totalOutputChannels);
}

inline std::array<int, SpatialRenderer::NUM_SPEAKERS> readCalibrationSpeakerRouting (
    const juce::AudioProcessorValueTreeState& apvts)
{
    std::array<int, SpatialRenderer::NUM_SPEAKERS> routing { 1, 2, 3, 4 };

    if (auto* p = apvts.getRawParameterValue ("cal_spk1_out")) routing[0] = static_cast<int> (std::lround (p->load()));
    if (auto* p = apvts.getRawParameterValue ("cal_spk2_out")) routing[1] = static_cast<int> (std::lround (p->load()));
    if (auto* p = apvts.getRawParameterValue ("cal_spk3_out")) routing[2] = static_cast<int> (std::lround (p->load()));
    if (auto* p = apvts.getRawParameterValue ("cal_spk4_out")) routing[3] = static_cast<int> (std::lround (p->load()));

    for (auto& channel : routing)
        channel = juce::jlimit (1, 8, channel);

    return routing;
}

inline int readDiscreteParameterIndex (const juce::AudioProcessorValueTreeState& apvts,
                                       const char* parameterId,
                                       int minValue,
                                       int maxValue,
                                       int fallbackValue) noexcept
{
    if (auto* value = apvts.getRawParameterValue (parameterId))
        return juce::jlimit (minValue, maxValue, static_cast<int> (std::lround (value->load())));

    return juce::jlimit (minValue, maxValue, fallbackValue);
}

inline void setIntegerParameterValueNotifyingHost (juce::AudioProcessorValueTreeState& apvts,
                                                   const char* parameterId,
                                                   int value)
{
    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (parameterId)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (value)));
}
} // namespace locusq::processor_core

