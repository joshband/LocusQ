#pragma once

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

//==============================================================================
/**
 * Supported interpolation curves between keyframes.
 */
enum class KeyframeCurve : uint8_t
{
    linear = 0,
    easeIn,
    easeOut,
    easeInOut,
    step
};

//==============================================================================
/**
 * Single keyframe point on a track.
 */
struct Keyframe
{
    double timeSeconds = 0.0;
    float value = 0.0f;
    KeyframeCurve curve = KeyframeCurve::linear;
};

//==============================================================================
/**
 * Track of keyframes for one parameter identifier.
 */
class KeyframeTrack
{
public:
    explicit KeyframeTrack (juce::String parameterId = {});

    void setParameterId (juce::String parameterId);
    const juce::String& getParameterId() const noexcept;

    void clear();
    bool empty() const noexcept;

    void setKeyframes (std::vector<Keyframe> newKeyframes);
    void addKeyframe (const Keyframe& keyframe);

    std::optional<float> evaluate (double timeSeconds) const;
    const std::vector<Keyframe>& getKeyframes() const noexcept;

private:
    juce::String parameterId;
    std::vector<Keyframe> keyframes;
};

//==============================================================================
/**
 * Multi-track animation timeline with internal clocking.
 */
class KeyframeTimeline
{
public:
    void prepare (double sampleRateHz) noexcept;
    void reset() noexcept;

    void clearTracks();
    void addOrReplaceTrack (KeyframeTrack track);

    bool hasTrack (const juce::String& parameterId) const;
    bool hasAnyTrack() const noexcept;

    std::optional<float> evaluateTrack (const juce::String& parameterId, double timeSeconds) const;
    std::optional<float> evaluateTrackAtCurrentTime (const juce::String& parameterId) const;

    void advance (double blockDurationSeconds) noexcept;
    void setCurrentTimeSeconds (double timeSeconds) noexcept;
    double getCurrentTimeSeconds() const noexcept;

    void setDurationSeconds (double newDurationSeconds) noexcept;
    double getDurationSeconds() const noexcept;

    void setLooping (bool shouldLoop) noexcept;
    bool isLooping() const noexcept;

    void setPlaybackRate (float newRate) noexcept;
    float getPlaybackRate() const noexcept;

    static float applyCurve (KeyframeCurve curve, float t) noexcept;
    const std::vector<KeyframeTrack>& getTracks() const noexcept;

private:
    static double normalizeTime (double timeSeconds, double durationSeconds, bool loop) noexcept;

    const KeyframeTrack* findTrack (const juce::String& parameterId) const;
    void refreshDurationFromTracks() noexcept;

    double sampleRateHz = 44100.0;
    double currentTimeSeconds = 0.0;
    double durationSeconds = 0.0;
    float playbackRate = 1.0f;
    bool looping = false;

    std::vector<KeyframeTrack> tracks;
};
