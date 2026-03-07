#include "../SpatialRenderer.h"

juce::String SpatialRenderer::getBundledPeqPresetFilenameForProfile (SpatialRenderer::HeadphoneDeviceProfile profile)
{
        switch (profile)
        {
            case HeadphoneDeviceProfile::AirPodsPro2:   return "airpods_pro_2_anc_on.yaml";
            case HeadphoneDeviceProfile::AirPodsPro3:   return "airpods_pro_3_anc_on.yaml";
            case HeadphoneDeviceProfile::SonyWH1000XM5: return "sony_wh1000xm5_anc_on.yaml";
            case HeadphoneDeviceProfile::Generic:
            case HeadphoneDeviceProfile::CustomSOFA:
            default: break;
        }

        return {};
    }


juce::File SpatialRenderer::resolveBundledPeqPresetFile (const juce::String& presetFilename) const
{
        if (presetFilename.isEmpty())
            return {};

        // NOTE: path traversal assumes macOS AU/VST3 bundle layout (Contents/MacOS/ + Contents/Resources/).
        // On Windows/Linux this resolves empty and cache entries remain invalid.
#if JUCE_MAC
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
            .getParentDirectory()
            .getSiblingFile ("Resources")
            .getChildFile ("eq_presets")
            .getChildFile (presetFilename);
#else
        juce::ignoreUnused (presetFilename);
        return {};
#endif
    }


void SpatialRenderer::preloadBundledPeqPresets()
{
        for (int profileIndex = 0; profileIndex < NUM_HEADPHONE_DEVICE_PROFILES; ++profileIndex)
        {
            auto& cacheEntry = bundledPeqPresets[static_cast<size_t> (profileIndex)];
            cacheEntry = BundledPeqPresetCacheEntry {};

            const auto profile = static_cast<HeadphoneDeviceProfile> (profileIndex);
            const auto presetFilename = getBundledPeqPresetFilenameForProfile (profile);
            if (presetFilename.isEmpty())
                continue;

            const auto presetFile = resolveBundledPeqPresetFile (presetFilename);
            if (! presetFile.existsAsFile())
                continue;

            cacheEntry.preset = locusq::headphone_dsp::loadHeadphonePreset (presetFile);
        }
    }


bool SpatialRenderer::tryBuildListenerOrientationFromCoordinateSpace (const IPLCoordinateSpace3& coordinateSpace,
                                                            SpatialRenderer::ListenerOrientation& orientation) noexcept
{
        orientation.right = { coordinateSpace.right.x, coordinateSpace.right.y, coordinateSpace.right.z };
        orientation.up = { coordinateSpace.up.x, coordinateSpace.up.y, coordinateSpace.up.z };
        orientation.ahead = { coordinateSpace.ahead.x, coordinateSpace.ahead.y, coordinateSpace.ahead.z };

        if (! locusq::spatial_headphone_pose::normalizeVector3 (orientation.right)
            || ! locusq::spatial_headphone_pose::normalizeVector3 (orientation.up)
            || ! locusq::spatial_headphone_pose::normalizeVector3 (orientation.ahead))
        {
            orientation = ListenerOrientation {};
            return false;
        }

        return true;
    }


void SpatialRenderer::setHeadPoseIdentityMix() noexcept
{
        locusq::spatial_headphone_pose::setHeadPoseIdentityMix (headPoseSpeakerMix);
    }


void SpatialRenderer::resetHeadPoseState() noexcept
{
        headPoseSnapshot = PoseSnapshot {};
        headPoseOrientation = ListenerOrientation {};
        headPoseValid = false;
        headPoseInternalBinauralActive = false;
        setHeadPoseIdentityMix();
    }


void SpatialRenderer::updateHeadPoseOrientationFromSnapshot() noexcept
{
        locusq::spatial_headphone_pose::updateOrientationFromQuaternion (
            headPoseSnapshot.qx,
            headPoseSnapshot.qy,
            headPoseSnapshot.qz,
            headPoseSnapshot.qw,
            headPoseOrientation);
    }


void SpatialRenderer::rebuildHeadPoseSpeakerMix() noexcept
{
        if (! headPoseValid)
        {
            setHeadPoseIdentityMix();
            return;
        }
        locusq::spatial_headphone_pose::buildSpeakerMixFromOrientation (
            headPoseOrientation,
            headPoseSpeakerMix);
    }


void SpatialRenderer::getHeadPoseAdjustedQuadSample (int sampleIndex, float& fl, float& fr, float& rr, float& rl) const noexcept
{
        if (! headPoseInternalBinauralActive || ! headPoseValid)
        {
            fl = accumBuffer.getSample (0, sampleIndex);
            fr = accumBuffer.getSample (1, sampleIndex);
            rr = accumBuffer.getSample (2, sampleIndex);
            rl = accumBuffer.getSample (3, sampleIndex);
            return;
        }

        const float sourceFl = accumBuffer.getSample (0, sampleIndex);
        const float sourceFr = accumBuffer.getSample (1, sampleIndex);
        const float sourceRr = accumBuffer.getSample (2, sampleIndex);
        const float sourceRl = accumBuffer.getSample (3, sampleIndex);
        locusq::spatial_headphone_pose::mixHeadPoseAdjustedQuadSample (
            headPoseSpeakerMix,
            sourceFl,
            sourceFr,
            sourceRr,
            sourceRl,
            fl,
            fr,
            rr,
            rl);
    }


bool SpatialRenderer::isStereoOrBinauralProfile (SpatialRenderer::SpatialOutputProfile profile) noexcept
{
        return locusq::spatial_profile_router::isStereoOrBinauralProfile (profile);
    }


SpatialRenderer::SpatialProfileResolution SpatialRenderer::resolveSpatialProfileForHost (int numOutputChannels) const noexcept
{
        const auto requested = static_cast<SpatialOutputProfile> (
            juce::jlimit (0, 11, requestedSpatialProfileIndex.load (std::memory_order_relaxed)));

        return locusq::spatial_profile_router::resolveSpatialProfileForHost (
            requested,
            numOutputChannels,
            NUM_SPEAKERS);
    }


int SpatialRenderer::ambisonicOrderForProfile (SpatialRenderer::SpatialOutputProfile profile) noexcept
{
        return locusq::spatial_profile_router::ambisonicOrderForProfile (profile);
    }


void SpatialRenderer::encodeAmbisonicFoaProxyFromQuad (float fl, float fr, float rr, float rl,
                                             float& w, float& x, float& y, float& z) noexcept
{
        locusq::spatial_profile_router::encodeAmbisonicFoaProxyFromQuad (fl, fr, rr, rl, w, x, y, z);
    }


void SpatialRenderer::decodeAmbisonicFoaProxyToStereo (float w, float x, float y, float z,
                                             float& left, float& right) noexcept
{
        locusq::spatial_profile_router::decodeAmbisonicFoaProxyToStereo (w, x, y, z, left, right);
    }


void SpatialRenderer::renderVirtual3dStereoSample (int sampleIndex, float& left, float& right) const noexcept
{
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        locusq::spatial_headphone_pose::renderVirtual3dStereoFromQuad (
            fl,
            fr,
            rr,
            rl,
            left,
            right);
    }


void SpatialRenderer::writeSurround521Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround521Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::writeSurround721Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround721Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::writeSurround742Sample (juce::AudioBuffer<float>& outputBuffer, int sampleIndex, float masterGain) const noexcept
{
        const float fl = accumBuffer.getSample (0, sampleIndex);
        const float fr = accumBuffer.getSample (1, sampleIndex);
        const float rr = accumBuffer.getSample (2, sampleIndex);
        const float rl = accumBuffer.getSample (3, sampleIndex);

        locusq::spatial_profile_router::writeSurround742Sample (
            outputBuffer, sampleIndex, masterGain, fl, fr, rr, rl);
    }


void SpatialRenderer::renderStereoDownmixSample (int sampleIndex, float& left, float& right) const noexcept
{
        float fl = 0.0f;
        float fr = 0.0f;
        float rr = 0.0f;
        float rl = 0.0f;
        getHeadPoseAdjustedQuadSample (sampleIndex, fl, fr, rr, rl);
        locusq::spatial_headphone_pose::renderStereoDownmixFromQuad (
            fl,
            fr,
            rr,
            rl,
            left,
            right);
    }


void SpatialRenderer::resetHeadphoneCompensationState() noexcept
{
        locusq::spatial_headphone_pose::resetHeadphoneCompensationState (
            headphoneCompLowStateLeft,
            headphoneCompLowStateRight);
    }


void SpatialRenderer::updateHeadphoneCompensationForProfile (SpatialRenderer::HeadphoneDeviceProfile profile) noexcept
{
        const auto config = locusq::spatial_headphone_pose::makeHeadphoneCompensationConfig (
            static_cast<int> (profile),
            currentSampleRate);
        headphoneCompLowAlpha = config.lowAlpha;
        headphoneCompLowGain = config.lowGain;
        headphoneCompHighGain = config.highGain;
        headphoneCompCrossfeed = config.crossfeed;
    }


void SpatialRenderer::applyHeadphoneProfileCompensation (float& left, float& right) noexcept
{
        const locusq::spatial_headphone_pose::HeadphoneCompensationConfig config
        {
            headphoneCompLowAlpha,
            headphoneCompLowGain,
            headphoneCompHighGain,
            headphoneCompCrossfeed
        };

        locusq::spatial_headphone_pose::applyHeadphoneCompensation (
            left,
            right,
            config,
            headphoneCompLowStateLeft,
            headphoneCompLowStateRight);
    }


float SpatialRenderer::calculateDistance (const Vec3& pos)
{
        return std::sqrt (pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    }


float SpatialRenderer::calculateAzimuth (const Vec3& pos)
{
        // Azimuth: angle in XZ plane from front (Z+), clockwise positive
        // atan2(x, z) gives angle from Z+ axis, positive clockwise when X+
        float az = std::atan2 (pos.x, pos.z) * (180.0f / 3.14159265358979323846f);
        return az;
    }


float SpatialRenderer::calculateElevation (const Vec3& pos)
{
        float hDist = std::sqrt (pos.x * pos.x + pos.z * pos.z);
        if (hDist < 0.001f && std::abs (pos.y) < 0.001f)
            return 0.0f;
        return std::atan2 (pos.y, hDist) * (180.0f / 3.14159265358979323846f);
    }
