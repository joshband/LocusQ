#include "Source/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
struct ProbeOptions
{
    int sampleRate = 48000;
    int blockSize = 512;
    int channels = 2;
    int blocks = 96;
    int spatialProfileIndex = 1;
    int headphoneModeIndex = 0;
    int headphoneProfileIndex = 0;
    juce::String profileAlias { "stereo" };
};

bool parseIntArg (const juce::String& value, int& out)
{
    const auto trimmed = value.trim();
    if (trimmed.isEmpty())
        return false;

    int start = 0;
    if (trimmed[0] == '-' || trimmed[0] == '+')
    {
        if (trimmed.length() == 1)
            return false;
        start = 1;
    }

    for (int i = start; i < trimmed.length(); ++i)
    {
        if (! juce::CharacterFunctions::isDigit (trimmed[i]))
            return false;
    }

    out = trimmed.getIntValue();
    return true;
}

float choiceToNormalised (int index, int choiceCount)
{
    const auto clampedIndex = juce::jlimit (0, choiceCount - 1, index);
    if (choiceCount <= 1)
        return 0.0f;

    return static_cast<float> (clampedIndex) / static_cast<float> (choiceCount - 1);
}

void setNormalisedParam (LocusQAudioProcessor& processor, const char* paramId, float normalised)
{
    if (auto* parameter = processor.apvts.getParameter (paramId))
        parameter->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalised));
}

juce::String getStringProperty (const juce::var& rootVar, const juce::Identifier& key, const juce::String& fallback)
{
    if (const auto* root = rootVar.getDynamicObject())
        return root->getProperty (key).toString();

    return fallback;
}

bool getBoolProperty (const juce::var& rootVar, const juce::Identifier& key, bool fallback)
{
    if (const auto* root = rootVar.getDynamicObject())
        return static_cast<bool> (root->getProperty (key));

    return fallback;
}

int getIntProperty (const juce::var& rootVar, const juce::Identifier& key, int fallback)
{
    if (const auto* root = rootVar.getDynamicObject())
        return static_cast<int> (root->getProperty (key));

    return fallback;
}

bool applyProfileAliasDefaults (const juce::String& alias, ProbeOptions& options)
{
    const auto profile = alias.trim().toLowerCase();
    if (profile == "mono")
    {
        options.spatialProfileIndex = 1;  // stereo_2_0 renderer path on 1ch host lane.
        options.channels = 1;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "stereo")
    {
        options.spatialProfileIndex = 1;
        options.channels = 2;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "quadraphonic")
    {
        options.spatialProfileIndex = 2;
        options.channels = 4;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "5.1")
    {
        options.spatialProfileIndex = 3;
        options.channels = 8;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "7.1")
    {
        options.spatialProfileIndex = 4;
        options.channels = 10;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "binaural_generic")
    {
        options.spatialProfileIndex = 9;  // virtual_3d_stereo
        options.channels = 2;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "binaural_steam")
    {
        options.spatialProfileIndex = 9;  // virtual_3d_stereo
        options.channels = 2;
        options.headphoneModeIndex = 1;
        return true;
    }

    if (profile == "ambisonic_1st")
    {
        options.spatialProfileIndex = 6;
        options.channels = 4;
        options.headphoneModeIndex = 0;
        return true;
    }

    if (profile == "ambisonic_3rd")
    {
        options.spatialProfileIndex = 7;
        options.channels = 16;
        options.headphoneModeIndex = 0;
        return true;
    }

    return false;
}

bool parseArgs (int argc, char* argv[], ProbeOptions& options, juce::String& error)
{
    for (int i = 1; i < argc; ++i)
    {
        const juce::String arg { argv[i] };

        auto nextValue = [&]() -> juce::String
        {
            if (i + 1 >= argc)
                return {};
            ++i;
            return juce::String { argv[i] };
        };

        if (arg == "--profile")
        {
            const auto value = nextValue();
            if (value.isEmpty() || ! applyProfileAliasDefaults (value, options))
            {
                error = "invalid --profile alias";
                return false;
            }

            options.profileAlias = value.trim().toLowerCase();
            continue;
        }

        if (arg == "--sample-rate")
        {
            if (! parseIntArg (nextValue(), options.sampleRate))
            {
                error = "invalid --sample-rate";
                return false;
            }
            continue;
        }

        if (arg == "--block-size")
        {
            if (! parseIntArg (nextValue(), options.blockSize))
            {
                error = "invalid --block-size";
                return false;
            }
            continue;
        }

        if (arg == "--channels")
        {
            if (! parseIntArg (nextValue(), options.channels))
            {
                error = "invalid --channels";
                return false;
            }
            continue;
        }

        if (arg == "--blocks")
        {
            if (! parseIntArg (nextValue(), options.blocks))
            {
                error = "invalid --blocks";
                return false;
            }
            continue;
        }

        if (arg == "--spatial-profile-index")
        {
            if (! parseIntArg (nextValue(), options.spatialProfileIndex))
            {
                error = "invalid --spatial-profile-index";
                return false;
            }
            continue;
        }

        if (arg == "--headphone-mode-index")
        {
            if (! parseIntArg (nextValue(), options.headphoneModeIndex))
            {
                error = "invalid --headphone-mode-index";
                return false;
            }
            continue;
        }

        if (arg == "--headphone-profile-index")
        {
            if (! parseIntArg (nextValue(), options.headphoneProfileIndex))
            {
                error = "invalid --headphone-profile-index";
                return false;
            }
            continue;
        }

        if (arg == "--help")
        {
            std::cout << "Usage: locusq_bl018_profile_probe "
                         "[--profile <mono|stereo|quadraphonic|5.1|7.1|binaural_generic|binaural_steam|ambisonic_1st|ambisonic_3rd>] "
                         "[--sample-rate <hz>] [--block-size <n>] [--channels <n>] [--blocks <n>] "
                         "[--spatial-profile-index <0..11>] [--headphone-mode-index <0..1>] "
                         "[--headphone-profile-index <0..3>]\n";
            std::exit (0);
        }

        error = "unknown argument: " + arg;
        return false;
    }

    if (options.sampleRate <= 0 || options.blockSize <= 0 || options.channels <= 0 || options.blocks <= 0)
    {
        error = "sample-rate/block-size/channels/blocks must be > 0";
        return false;
    }

    options.spatialProfileIndex = juce::jlimit (0, 11, options.spatialProfileIndex);
    options.headphoneModeIndex = juce::jlimit (0, 1, options.headphoneModeIndex);
    options.headphoneProfileIndex = juce::jlimit (0, 3, options.headphoneProfileIndex);
    options.channels = juce::jlimit (1, 16, options.channels);
    options.blocks = juce::jlimit (1, 4096, options.blocks);

    return true;
}
} // namespace

int main (int argc, char* argv[])
{
    ProbeOptions options;
    juce::String parseError;
    if (! parseArgs (argc, argv, options, parseError))
    {
        std::cerr << "ERROR: " << parseError << "\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    LocusQAudioProcessor emitter;
    LocusQAudioProcessor renderer;

    setNormalisedParam (emitter, "mode", choiceToNormalised (1, 3));   // Emitter
    setNormalisedParam (renderer, "mode", choiceToNormalised (2, 3));  // Renderer

    emitter.setRateAndBufferSizeDetails (options.sampleRate, options.blockSize);
    renderer.setRateAndBufferSizeDetails (options.sampleRate, options.blockSize);
    emitter.prepareToPlay (static_cast<double> (options.sampleRate), options.blockSize);
    renderer.prepareToPlay (static_cast<double> (options.sampleRate), options.blockSize);

    // Deterministic baseline emitter state.
    setNormalisedParam (emitter, "emit_mute", 0.0f);
    setNormalisedParam (emitter, "emit_gain", 0.62f);
    setNormalisedParam (emitter, "emit_spread", 0.10f);
    setNormalisedParam (emitter, "emit_directivity", 0.35f);
    setNormalisedParam (emitter, "pos_azimuth", 0.73f);
    setNormalisedParam (emitter, "pos_elevation", 0.44f);
    setNormalisedParam (emitter, "pos_distance", 0.12f);
    setNormalisedParam (emitter, "phys_enable", 0.0f);
    setNormalisedParam (emitter, "anim_enable", 0.0f);

    setNormalisedParam (renderer, "rend_master_gain", 0.50f);
    setNormalisedParam (renderer, "rend_quality", 1.0f);
    setNormalisedParam (renderer, "rend_room_enable", 0.0f);
    setNormalisedParam (renderer, "rend_doppler", 0.0f);
    setNormalisedParam (renderer, "rend_air_absorb", 0.0f);
    setNormalisedParam (renderer, "rend_spatial_profile", choiceToNormalised (options.spatialProfileIndex, 12));
    setNormalisedParam (renderer, "rend_headphone_mode", choiceToNormalised (options.headphoneModeIndex, 2));
    setNormalisedParam (renderer, "rend_headphone_profile", choiceToNormalised (options.headphoneProfileIndex, 4));
    renderer.primeRendererStateFromCurrentParameters();

    juce::AudioBuffer<float> emitterBuffer (options.channels, options.blockSize);
    juce::AudioBuffer<float> rendererBuffer (options.channels, options.blockSize);
    juce::MidiBuffer midiBuffer;

    double phase = 0.0;
    const double phaseIncrement = juce::MathConstants<double>::twoPi * 220.0 / static_cast<double> (options.sampleRate);

    for (int block = 0; block < options.blocks; ++block)
    {
        for (int ch = 0; ch < options.channels; ++ch)
        {
            auto* writePtr = emitterBuffer.getWritePointer (ch);
            for (int sample = 0; sample < options.blockSize; ++sample)
            {
                const float tone = 0.18f * std::sin (phase);
                writePtr[sample] = tone;
                phase += phaseIncrement;
            }
        }

        emitter.processBlock (emitterBuffer, midiBuffer);
        midiBuffer.clear();

        rendererBuffer.clear();
        renderer.processBlock (rendererBuffer, midiBuffer);
        midiBuffer.clear();
    }

    const auto sceneJson = renderer.getSceneStateJSON();
    const auto sceneState = juce::JSON::parse (sceneJson);
    if (! sceneState.isObject())
    {
        std::cerr << "ERROR: failed to parse scene state JSON\n";
        return 3;
    }

    juce::DynamicObject::Ptr out (new juce::DynamicObject());
    out->setProperty ("schema", "locusq-bl018-profile-diagnostics-v1");
    out->setProperty ("profileAlias", options.profileAlias);
    out->setProperty ("sampleRate", options.sampleRate);
    out->setProperty ("blockSize", options.blockSize);
    out->setProperty ("channels", options.channels);
    out->setProperty ("blocks", options.blocks);
    out->setProperty ("requestedSpatialProfileIndex", options.spatialProfileIndex);
    out->setProperty ("requestedHeadphoneModeIndex", options.headphoneModeIndex);
    out->setProperty ("requestedHeadphoneProfileIndex", options.headphoneProfileIndex);
    out->setProperty ("rendererSpatialProfileRequested",
                      getStringProperty (sceneState, "rendererSpatialProfileRequested", "unknown"));
    out->setProperty ("rendererSpatialProfileActive",
                      getStringProperty (sceneState, "rendererSpatialProfileActive", "unknown"));
    out->setProperty ("rendererSpatialProfileStage",
                      getStringProperty (sceneState, "rendererSpatialProfileStage", "unknown"));
    out->setProperty ("rendererHeadphoneModeRequested",
                      getStringProperty (sceneState, "rendererHeadphoneModeRequested", "unknown"));
    out->setProperty ("rendererHeadphoneModeActive",
                      getStringProperty (sceneState, "rendererHeadphoneModeActive", "unknown"));
    out->setProperty ("rendererHeadphoneProfileRequested",
                      getStringProperty (sceneState, "rendererHeadphoneProfileRequested", "unknown"));
    out->setProperty ("rendererHeadphoneProfileActive",
                      getStringProperty (sceneState, "rendererHeadphoneProfileActive", "unknown"));
    out->setProperty ("rendererSteamAudioCompiled",
                      getBoolProperty (sceneState, "rendererSteamAudioCompiled", false));
    out->setProperty ("rendererSteamAudioAvailable",
                      getBoolProperty (sceneState, "rendererSteamAudioAvailable", false));
    out->setProperty ("rendererSteamAudioInitStage",
                      getStringProperty (sceneState, "rendererSteamAudioInitStage", "unknown"));
    out->setProperty ("rendererSteamAudioInitErrorCode",
                      getIntProperty (sceneState, "rendererSteamAudioInitErrorCode", 0));
    out->setProperty ("outputLayout",
                      getStringProperty (sceneState, "outputLayout", "unknown"));
    out->setProperty ("outputChannels",
                      getIntProperty (sceneState, "outputChannels", options.channels));

    std::cout << juce::JSON::toString (juce::var (out.get()), true) << "\n";

    emitter.releaseResources();
    renderer.releaseResources();

    return 0;
}
