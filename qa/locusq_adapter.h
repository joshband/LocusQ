// locusq_adapter.h
// Audio DSP QA Harness adapter for LocusQ
//
// Two adapters:
//   LocusQEmitterAdapter  - single Emitter instance; validates passthrough
//   LocusQSpatialAdapter  - compound Emitter+Renderer pair; validates spatialization
//
// Parameter mappings are documented per-adapter below.

#pragma once

#include "core/dsp_under_test.h"
#include "core/effect_capabilities.h"
#include "Source/PluginProcessor.h"

#include <array>
#include <cstdint>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace locusq {
namespace qa {

//==============================================================================
/**
 * LocusQEmitterAdapter
 *
 * Wraps a single LocusQAudioProcessor in Emitter mode.
 * Validates: passthrough fidelity, RT safety, parameter handling.
 *
 * Parameter mapping (normalized [0,1]):
 *   0: emit_gain   (-60..+12 dB)
 *   1: emit_mute   (bool: >0.5 = muted)
 *   2: emit_spread (0..1)
 *   3: pos_azimuth (-180..+180 degrees)
 *   4: pos_distance (0..50 metres)
 */
class LocusQEmitterAdapter : public ::qa::DspUnderTest
{
public:
    LocusQEmitterAdapter();
    ~LocusQEmitterAdapter() override = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void release() override;
    void reset() noexcept override;

    void processBlock(float** channelData, int numChannels, int numSamples) noexcept override;

    void setParameter(int index, ::qa::NormalizedParam value) noexcept override;
    int getParameterCount() const noexcept override { return kNumParameters; }
    const char* getParameterName(int index) const override;

    bool getCapabilities(::qa::EffectCapabilities& out) const override;
    ::qa::OptionalFeatures getOptionalFeatures() const override;
    bool saveState(std::vector<std::uint8_t>& outState) const;
    bool loadState(const std::vector<std::uint8_t>& state);

private:
    static constexpr int kNumParameters = 5;

    std::unique_ptr<LocusQAudioProcessor> processor_;
    juce::AudioBuffer<float> audioBuffer_;
    juce::MidiBuffer midiBuffer_;

    void setJuceParam(const char* id, float normalized);
};

//==============================================================================
/**
 * LocusQSpatialAdapter
 *
 * Compound adapter: creates one Emitter + one Renderer instance that share
 * the process-wide SceneGraph singleton (exactly as in a real DAW session).
 *
 * processBlock() order:
 *   1. Input audio → Emitter (publishes audio + position to SceneGraph)
 *   2. Renderer reads SceneGraph → produces spatialized stereo output
 *   3. Renderer output replaces the input buffer
 *
 * Parameter mapping (normalized [0,1]):
 *   0:  pos_azimuth          (-180..+180 degrees, 0.5 = 0°)
 *   1:  pos_elevation        (-90..+90 degrees, 0.5 = 0°)
 *   2:  pos_distance         (0..50 metres)
 *   3:  emit_gain            (-60..+12 dB)
 *   4:  emit_mute            (bool: >0.5 = muted)
 *   5:  rend_master_gain     (-60..+12 dB)
 *   6:  rend_distance_model  (choice: InvSq/Linear/Log/Custom)
 *   7:  rend_air_absorb      (bool: >0.5 = enabled)
 *   8:  emit_spread          (0..1)
 *   9:  emit_directivity     (0..1)
 *   10: emit_dir_azimuth     (-180..+180)
 *   11: emit_dir_elevation   (-90..+90)
 *   12: rend_quality         (choice: 0=Draft, 1=Final)
 *   13: rend_doppler         (bool: >0.5 = enabled)
 *   14: rend_doppler_scale   (0..5)
 *   15: rend_room_enable     (bool: >0.5 = enabled)
 *   16: rend_room_mix        (0..1)
 *   17: rend_room_size       (0.5..5.0)
 *   18: rend_room_damping    (0..1)
 *   19: rend_room_er_only    (bool: >0.5 = enabled)
 *   20: phys_enable          (bool: >0.5 = enabled)
 *   21: phys_vel_x           (-50..+50)
 *   22: phys_vel_y           (-50..+50)
 *   23: phys_vel_z           (-50..+50)
 *   24: phys_throw           (one-shot gate)
 *   25: phys_drag            (0..10)
 *   26: phys_gravity         (-20..+20)
 *   27: anim_enable          (bool: >0.5 = enabled)
 *   28: anim_mode            (choice: 0=DAW, 1=Internal)
 *   29: anim_loop            (bool: >0.5 = loop enabled)
 *   30: anim_speed           (0.1..10.0)
 *   31: anim_sync            (bool: >0.5 = transport sync)
 *   32: qa_emitter_instances (1..16 emitters)
 *   33: qa_snapshot_migration_mode (0=off, 0.5=legacy-strip, 1.0=force-quad-layout-metadata)
 */
class LocusQSpatialAdapter : public ::qa::DspUnderTest
{
public:
    LocusQSpatialAdapter();
    ~LocusQSpatialAdapter() override = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void release() override;
    void reset() noexcept override;

    void processBlock(float** channelData, int numChannels, int numSamples) noexcept override;

    void setParameter(int index, ::qa::NormalizedParam value) noexcept override;
    int getParameterCount() const noexcept override { return kNumParameters; }
    const char* getParameterName(int index) const override;

    bool getCapabilities(::qa::EffectCapabilities& out) const override;
    ::qa::OptionalFeatures getOptionalFeatures() const override;
    bool saveState(std::vector<std::uint8_t>& outState) const;
    bool loadState(const std::vector<std::uint8_t>& state);

private:
    static constexpr int kNumParameters = 34;
    static constexpr int kMaxQaEmitters = 16;

    // Emitters + renderer sharing the same process-wide SceneGraph singleton
    std::vector<std::unique_ptr<LocusQAudioProcessor>> emitters_;
    std::unique_ptr<LocusQAudioProcessor> renderer_;

    std::vector<juce::AudioBuffer<float>> emitterBuffers_;
    juce::AudioBuffer<float> rendererBuffer_;
    juce::MidiBuffer midiBuffer_;

    std::array<float, kNumParameters> paramValues_ {};
    std::array<bool, kNumParameters> paramTouched_ {};

    double preparedSampleRate_ = 48000.0;
    int preparedBlockSize_ = 512;
    int preparedNumChannels_ = 2;
    int activeEmitterCount_ = 1;
    bool prepared_ = false;
    float snapshotMigrationMode_ = 0.0f;

    static int normalizedToEmitterCount(float normalized) noexcept;
    void rebuildEmitters(int emitterCount);
    void applyStoredParametersToEmitter(LocusQAudioProcessor& emitter);
    void applyStoredParametersToRenderer();

    void setEmitterParam(LocusQAudioProcessor& emitter, const char* id, float normalized);
    void setEmitterParamForAll(const char* id, float normalized);
    void setRendererParam(const char* id, float normalized);
};

} // namespace qa
} // namespace locusq
