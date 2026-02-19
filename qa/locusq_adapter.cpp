// locusq_adapter.cpp
// Audio DSP QA Harness adapter for LocusQ

#include "locusq_adapter.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

enum class SnapshotMigrationMode
{
    disabled,
    legacyStripLayoutMetadata,
    forceQuadLayoutMetadata
};

SnapshotMigrationMode decodeSnapshotMigrationMode(float normalized) noexcept
{
    if (normalized >= 0.67f)
        return SnapshotMigrationMode::forceQuadLayoutMetadata;
    if (normalized >= 0.34f)
        return SnapshotMigrationMode::legacyStripLayoutMetadata;
    return SnapshotMigrationMode::disabled;
}

bool rewritePluginSnapshotState(juce::MemoryBlock& stateBlock, SnapshotMigrationMode mode)
{
    if (mode == SnapshotMigrationMode::disabled)
        return true;

    std::unique_ptr<juce::XmlElement> stateXml(juce::AudioProcessor::getXmlFromBinary(
        stateBlock.getData(), static_cast<int>(stateBlock.getSize())));
    if (!stateXml)
        return false;

    auto stateTree = juce::ValueTree::fromXml(*stateXml);
    if (!stateTree.isValid())
        return false;

    switch (mode)
    {
        case SnapshotMigrationMode::legacyStripLayoutMetadata:
            stateTree.removeProperty("locusq_snapshot_schema", nullptr);
            stateTree.removeProperty("locusq_output_layout", nullptr);
            stateTree.removeProperty("locusq_output_channels", nullptr);
            break;

        case SnapshotMigrationMode::forceQuadLayoutMetadata:
            stateTree.setProperty("locusq_snapshot_schema", "locusq-state-v2", nullptr);
            stateTree.setProperty("locusq_output_layout", "quad", nullptr);
            stateTree.setProperty("locusq_output_channels", 4, nullptr);
            break;

        case SnapshotMigrationMode::disabled:
            break;
    }

    std::unique_ptr<juce::XmlElement> migratedXml(stateTree.createXml());
    if (!migratedXml)
        return false;

    juce::MemoryBlock migratedBlock;
    juce::AudioProcessor::copyXmlToBinary(*migratedXml, migratedBlock);
    if (migratedBlock.getSize() == 0)
        return false;

    stateBlock = std::move(migratedBlock);
    return true;
}

} // namespace

namespace locusq {
namespace qa {

//==============================================================================
// LocusQEmitterAdapter
//==============================================================================

LocusQEmitterAdapter::LocusQEmitterAdapter()
    : processor_(std::make_unique<LocusQAudioProcessor>())
{
    // Force Emitter mode (index 1 in the "Calibrate/Emitter/Renderer" choice)
    setJuceParam("mode", 1.0f / 2.0f);  // normalized 0.5 = index 1 of 3 choices
}

void LocusQEmitterAdapter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    processor_->setRateAndBufferSizeDetails(sampleRate, maxBlockSize);
    processor_->prepareToPlay(sampleRate, maxBlockSize);

    audioBuffer_.setSize(numChannels, maxBlockSize, false, true, true);
    audioBuffer_.clear();
    midiBuffer_.clear();
}

void LocusQEmitterAdapter::release()
{
    processor_->releaseResources();
    audioBuffer_.setSize(0, 0);
}

void LocusQEmitterAdapter::reset() noexcept
{
    audioBuffer_.clear();
    midiBuffer_.clear();
}

void LocusQEmitterAdapter::processBlock(float** channelData, int numChannels, int numSamples) noexcept
{
    // Copy input into JUCE buffer
    for (int ch = 0; ch < numChannels; ++ch)
        if (channelData[ch])
            std::memcpy(audioBuffer_.getWritePointer(ch), channelData[ch],
                        static_cast<size_t>(numSamples) * sizeof(float));

    processor_->processBlock(audioBuffer_, midiBuffer_);
    midiBuffer_.clear();

    // Copy output back (emitter passes audio through)
    for (int ch = 0; ch < numChannels; ++ch)
        if (channelData[ch])
            std::memcpy(channelData[ch], audioBuffer_.getReadPointer(ch),
                        static_cast<size_t>(numSamples) * sizeof(float));
}

void LocusQEmitterAdapter::setParameter(int index, ::qa::NormalizedParam value) noexcept
{
    switch (index)
    {
        case 0: setJuceParam("emit_gain",    value); break;
        case 1: setJuceParam("emit_mute",    value); break;
        case 2: setJuceParam("emit_spread",  value); break;
        case 3: setJuceParam("pos_azimuth",  value); break;
        case 4: setJuceParam("pos_distance", value); break;
        default: break;
    }
}

const char* LocusQEmitterAdapter::getParameterName(int index) const
{
    static const char* names[] = {
        "emit_gain", "emit_mute", "emit_spread", "pos_azimuth", "pos_distance"
    };
    if (index >= 0 && index < kNumParameters) return names[index];
    return nullptr;
}

bool LocusQEmitterAdapter::getCapabilities(::qa::EffectCapabilities& out) const
{
    out.effectTypes = ::qa::EffectType::UTILITY;
    out.behaviors   = ::qa::BehaviorFlag::NONE;
    out.description = "LocusQ Emitter: spatial audio source with passthrough";
    return true;
}

::qa::OptionalFeatures LocusQEmitterAdapter::getOptionalFeatures() const
{
    return { true,   // supportsReset
             false,  // supportsMidiInput
             false,  // supportsMidiOutput
             false,  // supportsTransport
             true,   // supportsCapabilities
             false }; // supportsRoutingIntrospection
}

bool LocusQEmitterAdapter::saveState(std::vector<std::uint8_t>& outState) const
{
    outState.clear();
    if (!processor_)
        return false;

    juce::MemoryBlock stateBlock;
    processor_->getStateInformation(stateBlock);

    if (stateBlock.getSize() == 0)
        return false;

    outState.resize(stateBlock.getSize());
    std::memcpy(outState.data(), stateBlock.getData(), stateBlock.getSize());
    return true;
}

bool LocusQEmitterAdapter::loadState(const std::vector<std::uint8_t>& state)
{
    if (!processor_ || state.empty())
        return false;

    processor_->setStateInformation(state.data(), static_cast<int>(state.size()));
    return true;
}

void LocusQEmitterAdapter::setJuceParam(const char* id, float normalized)
{
    if (auto* p = processor_->apvts.getParameter(id))
        p->setValueNotifyingHost(normalized);
}

//==============================================================================
// LocusQSpatialAdapter
//==============================================================================

LocusQSpatialAdapter::LocusQSpatialAdapter()
{
    paramValues_.fill(0.0f);
    paramTouched_.fill(false);
}

int LocusQSpatialAdapter::normalizedToEmitterCount(float normalized) noexcept
{
    const auto clamped = juce::jlimit(0.0f, 1.0f, normalized);
    const auto scaled = static_cast<int>(std::lround(clamped * static_cast<float>(kMaxQaEmitters - 1)));
    return juce::jlimit(1, kMaxQaEmitters, 1 + scaled);
}

void LocusQSpatialAdapter::rebuildEmitters(int emitterCount)
{
    const int clampedCount = juce::jlimit(1, kMaxQaEmitters, emitterCount);
    activeEmitterCount_ = clampedCount;

    for (auto& emitter : emitters_)
        if (emitter)
            emitter->releaseResources();

    emitters_.clear();
    emitterBuffers_.clear();
    emitters_.reserve(static_cast<size_t>(activeEmitterCount_));
    emitterBuffers_.reserve(static_cast<size_t>(activeEmitterCount_));

    if (!prepared_)
        return;

    for (int i = 0; i < activeEmitterCount_; ++i)
    {
        auto emitter = std::make_unique<LocusQAudioProcessor>();

        if (auto* mode = emitter->apvts.getParameter("mode"))
            mode->setValueNotifyingHost(1.0f / 2.0f); // Emitter

        emitter->setRateAndBufferSizeDetails(preparedSampleRate_, preparedBlockSize_);
        emitter->prepareToPlay(preparedSampleRate_, preparedBlockSize_);
        applyStoredParametersToEmitter(*emitter);

        emitters_.push_back(std::move(emitter));

        juce::AudioBuffer<float> emitterBuffer;
        emitterBuffer.setSize(preparedNumChannels_, preparedBlockSize_, false, true, true);
        emitterBuffer.clear();
        emitterBuffers_.push_back(std::move(emitterBuffer));
    }
}

void LocusQSpatialAdapter::applyStoredParametersToEmitter(LocusQAudioProcessor& emitter)
{
    auto apply = [this, &emitter](int index, const char* id)
    {
        if (paramTouched_[static_cast<size_t>(index)])
            setEmitterParam(emitter, id, paramValues_[static_cast<size_t>(index)]);
    };

    apply(0,  "pos_azimuth");
    apply(1,  "pos_elevation");
    apply(2,  "pos_distance");
    apply(3,  "emit_gain");
    apply(4,  "emit_mute");
    apply(8,  "emit_spread");
    apply(9,  "emit_directivity");
    apply(10, "emit_dir_azimuth");
    apply(11, "emit_dir_elevation");
    apply(20, "phys_enable");
    apply(21, "phys_vel_x");
    apply(22, "phys_vel_y");
    apply(23, "phys_vel_z");
    apply(24, "phys_throw");
    apply(25, "phys_drag");
    apply(26, "phys_gravity");
    apply(27, "anim_enable");
    apply(28, "anim_mode");
    apply(29, "anim_loop");
    apply(30, "anim_speed");
    apply(31, "anim_sync");
}

void LocusQSpatialAdapter::applyStoredParametersToRenderer()
{
    if (!renderer_)
        return;

    auto apply = [this](int index, const char* id)
    {
        if (paramTouched_[static_cast<size_t>(index)])
            setRendererParam(id, paramValues_[static_cast<size_t>(index)]);
    };

    apply(5,  "rend_master_gain");
    apply(6,  "rend_distance_model");
    apply(7,  "rend_air_absorb");
    apply(12, "rend_quality");
    apply(13, "rend_doppler");
    apply(14, "rend_doppler_scale");
    apply(15, "rend_room_enable");
    apply(16, "rend_room_mix");
    apply(17, "rend_room_size");
    apply(18, "rend_room_damping");
    apply(19, "rend_room_er_only");
}

void LocusQSpatialAdapter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    preparedSampleRate_ = sampleRate;
    preparedBlockSize_ = maxBlockSize;
    preparedNumChannels_ = numChannels;

    if (!renderer_)
        renderer_ = std::make_unique<LocusQAudioProcessor>();

    if (auto* mode = renderer_->apvts.getParameter("mode"))
        mode->setValueNotifyingHost(1.0f); // Renderer

    renderer_->setRateAndBufferSizeDetails(sampleRate, maxBlockSize);
    renderer_->prepareToPlay(sampleRate, maxBlockSize);

    rendererBuffer_.setSize(numChannels, maxBlockSize, false, true, true);
    rendererBuffer_.clear();
    midiBuffer_.clear();

    prepared_ = true;
    rebuildEmitters(activeEmitterCount_);
    applyStoredParametersToRenderer();
}

void LocusQSpatialAdapter::release()
{
    for (auto& emitter : emitters_)
        if (emitter)
            emitter->releaseResources();

    emitters_.clear();
    emitterBuffers_.clear();

    if (renderer_)
        renderer_->releaseResources();

    rendererBuffer_.setSize(0, 0);
    prepared_ = false;
}

void LocusQSpatialAdapter::reset() noexcept
{
    for (auto& emitterBuffer : emitterBuffers_)
        emitterBuffer.clear();

    rendererBuffer_.clear();
    midiBuffer_.clear();
}

void LocusQSpatialAdapter::processBlock(float** channelData, int numChannels, int numSamples) noexcept
{
    if (!renderer_ || emitters_.empty())
    {
        for (int ch = 0; ch < numChannels; ++ch)
            if (channelData[ch] != nullptr)
                std::memset(channelData[ch], 0, static_cast<size_t>(numSamples) * sizeof(float));
        return;
    }

    // Step 1: Run all emitters with identical input stimulus.
    for (size_t emitterIndex = 0; emitterIndex < emitters_.size(); ++emitterIndex)
    {
        auto& emitterBuffer = emitterBuffers_[emitterIndex];

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (channelData[ch] != nullptr)
            {
                std::memcpy(emitterBuffer.getWritePointer(ch), channelData[ch],
                            static_cast<size_t>(numSamples) * sizeof(float));
            }
        }

        emitters_[emitterIndex]->processBlock(emitterBuffer, midiBuffer_);
        midiBuffer_.clear();
    }

    // Step 2: Renderer reads all emitter slots and produces spatialized output.
    rendererBuffer_.clear();
    renderer_->processBlock(rendererBuffer_, midiBuffer_);
    midiBuffer_.clear();

    // Step 3: Copy renderer output to harness output buffer.
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (channelData[ch] != nullptr)
        {
            std::memcpy(channelData[ch], rendererBuffer_.getReadPointer(ch),
                        static_cast<size_t>(numSamples) * sizeof(float));
        }
    }
}

void LocusQSpatialAdapter::setParameter(int index, ::qa::NormalizedParam value) noexcept
{
    if (index < 0 || index >= kNumParameters)
        return;

    const auto normalized = juce::jlimit(0.0f, 1.0f, value);
    paramValues_[static_cast<size_t>(index)] = normalized;
    paramTouched_[static_cast<size_t>(index)] = true;

    switch (index)
    {
        case 0:  setEmitterParamForAll("pos_azimuth",         normalized); break;
        case 1:  setEmitterParamForAll("pos_elevation",       normalized); break;
        case 2:  setEmitterParamForAll("pos_distance",        normalized); break;
        case 3:  setEmitterParamForAll("emit_gain",           normalized); break;
        case 4:  setEmitterParamForAll("emit_mute",           normalized); break;
        case 5:  setRendererParam     ("rend_master_gain",    normalized); break;
        case 6:  setRendererParam     ("rend_distance_model", normalized); break;
        case 7:  setRendererParam     ("rend_air_absorb",     normalized); break;
        case 8:  setEmitterParamForAll("emit_spread",         normalized); break;
        case 9:  setEmitterParamForAll("emit_directivity",    normalized); break;
        case 10: setEmitterParamForAll("emit_dir_azimuth",    normalized); break;
        case 11: setEmitterParamForAll("emit_dir_elevation",  normalized); break;
        case 12: setRendererParam     ("rend_quality",        normalized); break;
        case 13: setRendererParam     ("rend_doppler",        normalized); break;
        case 14: setRendererParam     ("rend_doppler_scale",  normalized); break;
        case 15: setRendererParam     ("rend_room_enable",    normalized); break;
        case 16: setRendererParam     ("rend_room_mix",       normalized); break;
        case 17: setRendererParam     ("rend_room_size",      normalized); break;
        case 18: setRendererParam     ("rend_room_damping",   normalized); break;
        case 19: setRendererParam     ("rend_room_er_only",   normalized); break;
        case 20: setEmitterParamForAll("phys_enable",         normalized); break;
        case 21: setEmitterParamForAll("phys_vel_x",          normalized); break;
        case 22: setEmitterParamForAll("phys_vel_y",          normalized); break;
        case 23: setEmitterParamForAll("phys_vel_z",          normalized); break;
        case 24: setEmitterParamForAll("phys_throw",          normalized); break;
        case 25: setEmitterParamForAll("phys_drag",           normalized); break;
        case 26: setEmitterParamForAll("phys_gravity",        normalized); break;
        case 27: setEmitterParamForAll("anim_enable",         normalized); break;
        case 28: setEmitterParamForAll("anim_mode",           normalized); break;
        case 29: setEmitterParamForAll("anim_loop",           normalized); break;
        case 30: setEmitterParamForAll("anim_speed",          normalized); break;
        case 31: setEmitterParamForAll("anim_sync",           normalized); break;
        case 32:
            rebuildEmitters(normalizedToEmitterCount(normalized));
            break;
        case 33:
            snapshotMigrationMode_ = normalized;
            break;
        default:
            break;
    }
}

const char* LocusQSpatialAdapter::getParameterName(int index) const
{
    static const char* names[] = {
        "pos_azimuth", "pos_elevation", "pos_distance",
        "emit_gain", "emit_mute",
        "rend_master_gain", "rend_distance_model", "rend_air_absorb",
        "emit_spread", "emit_directivity", "emit_dir_azimuth", "emit_dir_elevation",
        "rend_quality", "rend_doppler", "rend_doppler_scale",
        "rend_room_enable", "rend_room_mix", "rend_room_size", "rend_room_damping",
        "rend_room_er_only", "phys_enable", "phys_vel_x", "phys_vel_y", "phys_vel_z",
        "phys_throw", "phys_drag", "phys_gravity",
        "anim_enable", "anim_mode", "anim_loop", "anim_speed", "anim_sync",
        "qa_emitter_instances", "qa_snapshot_migration_mode"
    };
    if (index >= 0 && index < kNumParameters) return names[index];
    return nullptr;
}

bool LocusQSpatialAdapter::getCapabilities(::qa::EffectCapabilities& out) const
{
    out.effectTypes = ::qa::EffectType::SPATIAL;
    out.behaviors   = ::qa::BehaviorFlag::STATEFUL;
    out.description = "LocusQ Spatial: multi-emitter+renderer quad spatialization";
    return true;
}

::qa::OptionalFeatures LocusQSpatialAdapter::getOptionalFeatures() const
{
    return { true,   // supportsReset
             false,  // supportsMidiInput
             false,  // supportsMidiOutput
             false,  // supportsTransport
             true,   // supportsCapabilities
             false }; // supportsRoutingIntrospection
}

bool LocusQSpatialAdapter::saveState(std::vector<std::uint8_t>& outState) const
{
    outState.clear();

    if (!prepared_ || !renderer_ || emitters_.empty())
        return false;

    juce::MemoryOutputStream stream;
    stream.writeInt(activeEmitterCount_);

    for (const auto& emitter : emitters_)
    {
        juce::MemoryBlock stateBlock;
        emitter->getStateInformation(stateBlock);
        stream.writeInt(static_cast<int>(stateBlock.getSize()));
        stream.write(stateBlock.getData(), stateBlock.getSize());
    }

    juce::MemoryBlock rendererState;
    renderer_->getStateInformation(rendererState);
    stream.writeInt(static_cast<int>(rendererState.getSize()));
    stream.write(rendererState.getData(), rendererState.getSize());

    outState.resize(stream.getDataSize());
    std::memcpy(outState.data(), stream.getData(), stream.getDataSize());
    return true;
}

bool LocusQSpatialAdapter::loadState(const std::vector<std::uint8_t>& state)
{
    if (!prepared_ || !renderer_ || state.empty())
        return false;

    juce::MemoryInputStream input(state.data(), state.size(), false);
    const auto migrationMode = decodeSnapshotMigrationMode(snapshotMigrationMode_);

    const int savedEmitterCount = input.readInt();
    if (savedEmitterCount < 1 || savedEmitterCount > kMaxQaEmitters)
        return false;

    if (savedEmitterCount != activeEmitterCount_)
        rebuildEmitters(savedEmitterCount);

    if (emitters_.size() != static_cast<size_t>(savedEmitterCount))
        return false;

    for (auto& emitter : emitters_)
    {
        const int stateSize = input.readInt();
        if (stateSize <= 0 || static_cast<size_t>(stateSize) > input.getNumBytesRemaining())
            return false;

        juce::MemoryBlock stateBlock(static_cast<size_t>(stateSize), true);
        if (input.read(stateBlock.getData(), static_cast<size_t>(stateSize)) != static_cast<size_t>(stateSize))
            return false;

        if (!rewritePluginSnapshotState(stateBlock, migrationMode))
            return false;

        emitter->setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
    }

    const int rendererStateSize = input.readInt();
    if (rendererStateSize <= 0 || static_cast<size_t>(rendererStateSize) > input.getNumBytesRemaining())
        return false;

    juce::MemoryBlock rendererState(static_cast<size_t>(rendererStateSize), true);
    if (input.read(rendererState.getData(), static_cast<size_t>(rendererStateSize)) != static_cast<size_t>(rendererStateSize))
        return false;

    if (!rewritePluginSnapshotState(rendererState, migrationMode))
        return false;

    renderer_->setStateInformation(rendererState.getData(), static_cast<int>(rendererState.getSize()));
    return true;
}

void LocusQSpatialAdapter::setEmitterParam(LocusQAudioProcessor& emitter, const char* id, float normalized)
{
    if (auto* parameter = emitter.apvts.getParameter(id))
        parameter->setValueNotifyingHost(normalized);
}

void LocusQSpatialAdapter::setEmitterParamForAll(const char* id, float normalized)
{
    for (auto& emitter : emitters_)
    {
        if (emitter)
            setEmitterParam(*emitter, id, normalized);
    }
}

void LocusQSpatialAdapter::setRendererParam(const char* id, float normalized)
{
    if (!renderer_)
        return;

    if (auto* parameter = renderer_->apvts.getParameter(id))
        parameter->setValueNotifyingHost(normalized);

    renderer_->primeRendererStateFromCurrentParameters();
}

} // namespace qa
} // namespace locusq
