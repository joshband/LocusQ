#include "KeyframeTimeline.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace
{
constexpr double kMinSegmentDuration = 1.0e-9;
}

//==============================================================================
KeyframeTrack::KeyframeTrack (juce::String newParameterId)
    : parameterId (std::move (newParameterId))
{
}

void KeyframeTrack::setParameterId (juce::String newParameterId)
{
    parameterId = std::move (newParameterId);
}

const juce::String& KeyframeTrack::getParameterId() const noexcept
{
    return parameterId;
}

void KeyframeTrack::clear()
{
    keyframes.clear();
}

bool KeyframeTrack::empty() const noexcept
{
    return keyframes.empty();
}

void KeyframeTrack::setKeyframes (std::vector<Keyframe> newKeyframes)
{
    keyframes = std::move (newKeyframes);
    std::sort (keyframes.begin(),
               keyframes.end(),
               [] (const Keyframe& lhs, const Keyframe& rhs)
               {
                   return lhs.timeSeconds < rhs.timeSeconds;
               });
}

void KeyframeTrack::addKeyframe (const Keyframe& keyframe)
{
    keyframes.push_back (keyframe);
    std::sort (keyframes.begin(),
               keyframes.end(),
               [] (const Keyframe& lhs, const Keyframe& rhs)
               {
                   return lhs.timeSeconds < rhs.timeSeconds;
               });
}

std::optional<float> KeyframeTrack::evaluate (double timeSeconds) const
{
    if (keyframes.empty())
        return std::nullopt;

    if (keyframes.size() == 1)
        return keyframes.front().value;

    if (timeSeconds <= keyframes.front().timeSeconds)
        return keyframes.front().value;

    if (timeSeconds >= keyframes.back().timeSeconds)
        return keyframes.back().value;

    const auto upper = std::upper_bound (keyframes.begin(),
                                         keyframes.end(),
                                         timeSeconds,
                                         [] (double value, const Keyframe& keyframe)
                                         {
                                             return value < keyframe.timeSeconds;
                                         });

    if (upper == keyframes.begin() || upper == keyframes.end())
        return keyframes.back().value;

    const auto& right = *upper;
    const auto& left = *std::prev (upper);

    const auto segmentDuration = right.timeSeconds - left.timeSeconds;
    if (segmentDuration <= kMinSegmentDuration)
        return right.value;

    const auto t = juce::jlimit (0.0f,
                                 1.0f,
                                 static_cast<float> ((timeSeconds - left.timeSeconds) / segmentDuration));
    const auto curveT = KeyframeTimeline::applyCurve (left.curve, t);
    return juce::jmap (curveT, left.value, right.value);
}

const std::vector<Keyframe>& KeyframeTrack::getKeyframes() const noexcept
{
    return keyframes;
}

//==============================================================================
void KeyframeTimeline::prepare (double newSampleRateHz) noexcept
{
    sampleRateHz = juce::jmax (1.0, newSampleRateHz);
    currentTimeSeconds = normalizeTime (currentTimeSeconds, durationSeconds, looping);
}

void KeyframeTimeline::reset() noexcept
{
    currentTimeSeconds = 0.0;
}

void KeyframeTimeline::clearTracks()
{
    tracks.clear();
    durationSeconds = 0.0;
    currentTimeSeconds = 0.0;
}

void KeyframeTimeline::addOrReplaceTrack (KeyframeTrack track)
{
    if (track.getParameterId().isEmpty())
        return;

    auto it = std::find_if (tracks.begin(),
                            tracks.end(),
                            [&track] (const KeyframeTrack& existing)
                            {
                                return existing.getParameterId() == track.getParameterId();
                            });

    if (it != tracks.end())
        *it = std::move (track);
    else
        tracks.push_back (std::move (track));

    refreshDurationFromTracks();
}

bool KeyframeTimeline::hasTrack (const juce::String& parameterId) const
{
    return findTrack (parameterId) != nullptr;
}

bool KeyframeTimeline::hasAnyTrack() const noexcept
{
    return ! tracks.empty();
}

std::optional<float> KeyframeTimeline::evaluateTrack (const juce::String& parameterId, double timeSeconds) const
{
    if (const auto* track = findTrack (parameterId))
        return track->evaluate (normalizeTime (timeSeconds, durationSeconds, looping));

    return std::nullopt;
}

std::optional<float> KeyframeTimeline::evaluateTrackAtCurrentTime (const juce::String& parameterId) const
{
    return evaluateTrack (parameterId, currentTimeSeconds);
}

void KeyframeTimeline::advance (double blockDurationSeconds) noexcept
{
    const auto safeDuration = juce::jmax (0.0, blockDurationSeconds);
    const auto safeRate = juce::jlimit (0.1f, 10.0f, playbackRate);
    setCurrentTimeSeconds (currentTimeSeconds + safeDuration * static_cast<double> (safeRate));
}

void KeyframeTimeline::setCurrentTimeSeconds (double timeSeconds) noexcept
{
    currentTimeSeconds = normalizeTime (timeSeconds, durationSeconds, looping);
}

double KeyframeTimeline::getCurrentTimeSeconds() const noexcept
{
    return currentTimeSeconds;
}

void KeyframeTimeline::setDurationSeconds (double newDurationSeconds) noexcept
{
    durationSeconds = juce::jmax (0.0, newDurationSeconds);
    currentTimeSeconds = normalizeTime (currentTimeSeconds, durationSeconds, looping);
}

double KeyframeTimeline::getDurationSeconds() const noexcept
{
    return durationSeconds;
}

void KeyframeTimeline::setLooping (bool shouldLoop) noexcept
{
    looping = shouldLoop;
    currentTimeSeconds = normalizeTime (currentTimeSeconds, durationSeconds, looping);
}

bool KeyframeTimeline::isLooping() const noexcept
{
    return looping;
}

void KeyframeTimeline::setPlaybackRate (float newRate) noexcept
{
    playbackRate = juce::jlimit (0.1f, 10.0f, newRate);
}

float KeyframeTimeline::getPlaybackRate() const noexcept
{
    return playbackRate;
}

float KeyframeTimeline::applyCurve (KeyframeCurve curve, float t) noexcept
{
    const auto x = juce::jlimit (0.0f, 1.0f, t);

    switch (curve)
    {
        case KeyframeCurve::linear:    return x;
        case KeyframeCurve::easeIn:    return x * x;
        case KeyframeCurve::easeOut:   return 1.0f - std::pow (1.0f - x, 2.0f);
        case KeyframeCurve::easeInOut: return (x < 0.5f) ? (2.0f * x * x)
                                                          : (1.0f - std::pow (-2.0f * x + 2.0f, 2.0f) * 0.5f);
        case KeyframeCurve::step:      return 0.0f;
    }

    return x;
}

const std::vector<KeyframeTrack>& KeyframeTimeline::getTracks() const noexcept
{
    return tracks;
}

double KeyframeTimeline::normalizeTime (double timeSeconds, double durationSeconds, bool loop) noexcept
{
    if (durationSeconds <= 0.0)
        return juce::jmax (0.0, timeSeconds);

    if (loop)
    {
        auto wrapped = std::fmod (timeSeconds, durationSeconds);
        if (wrapped < 0.0)
            wrapped += durationSeconds;
        return wrapped;
    }

    return juce::jlimit (0.0, durationSeconds, timeSeconds);
}

const KeyframeTrack* KeyframeTimeline::findTrack (const juce::String& parameterId) const
{
    const auto it = std::find_if (tracks.begin(),
                                  tracks.end(),
                                  [&parameterId] (const KeyframeTrack& track)
                                  {
                                      return track.getParameterId() == parameterId;
                                  });

    return it != tracks.end() ? std::addressof (*it) : nullptr;
}

void KeyframeTimeline::refreshDurationFromTracks() noexcept
{
    double longestTrack = 0.0;

    for (const auto& track : tracks)
    {
        if (! track.empty())
            longestTrack = juce::jmax (longestTrack, track.getKeyframes().back().timeSeconds);
    }

    durationSeconds = longestTrack;
    currentTimeSeconds = normalizeTime (currentTimeSeconds, durationSeconds, looping);
}
