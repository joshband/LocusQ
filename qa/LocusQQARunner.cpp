// LocusQ QA Harness - LocusQQARunner.cpp
//
// Shared runner_app-backed QA entrypoint implementation for LocusQ.

#include "LocusQQARunner.h"
#include "locusq_adapter.h"

#include "qa_runner_app/BaseQARunner.h"
#include "runners/performance_profiler.h"

#if defined(QA_HOST_RUNNER_AVAILABLE)
#include "runners/au_plugin_host.h"
#include "runners/host_runner.h"
#include "runners/plugin_host_interface.h"
#include "runners/vst3_plugin_host.h"
#endif

#include <juce_events/juce_events.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

std::unique_ptr<qa::DspUnderTest> createEmitterDut()
{
    return std::make_unique<locusq::qa::LocusQEmitterAdapter>();
}

std::unique_ptr<qa::DspUnderTest> createSpatialDut()
{
    return std::make_unique<locusq::qa::LocusQSpatialAdapter>();
}

std::unique_ptr<qa::DspUnderTest> createCalibrateDut()
{
    return std::make_unique<locusq::qa::LocusQCalibrateAdapter>();
}

struct HostRunnerOptions
{
    bool enabled = false;
    bool skeletonMode = false;
    std::string format;
    std::string pluginPath;
    std::string outputDir;
};

struct LocusQRunOptions : public qa::runner_app::RunOptions
{
    bool useSpatial = false;
    bool useCalibrate = false;
    std::string parseError;
    HostRunnerOptions hostRunner;
};

nlohmann::json mergedScenarioParameters(const qa::scenario::ScenarioSpec& scenario)
{
    nlohmann::json merged = nlohmann::json::object();

    if (scenario.defaultParameters.is_object())
        merged = scenario.defaultParameters;

    if (scenario.parameterVariations.is_object())
    {
        for (auto it = scenario.parameterVariations.begin(); it != scenario.parameterVariations.end(); ++it)
            merged[it.key()] = it.value();
    }

    return merged;
}

int resolveParameterIndex(const qa::DspUnderTest& dut,
                          const std::string& key,
                          int parameterCount)
{
    try
    {
        std::size_t parsed = 0;
        const int index = std::stoi(key, &parsed);
        if (parsed == key.size() && index >= 0 && index < parameterCount)
            return index;
    }
    catch (const std::exception&)
    {
        // Fall through to name lookup.
    }

    for (int i = 0; i < parameterCount; ++i)
    {
        if (const auto* name = dut.getParameterName(i); name != nullptr && key == name)
            return i;
    }

    return -1;
}

void applyScenarioParameters(qa::DspUnderTest& dut, const nlohmann::json& parameters)
{
    if (!parameters.is_object())
        return;

    const int parameterCount = dut.getParameterCount();
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
    {
        if (!it.value().is_number())
            continue;

        const int index = resolveParameterIndex(dut, it.key(), parameterCount);
        if (index < 0)
            continue;

        const float normalized = std::clamp(it.value().get<float>(), 0.0f, 1.0f);
        dut.setParameter(index, normalized);
    }
}

void attachProfilingMetrics(const qa::scenario::ScenarioSpec& scenario,
                            const qa::DutFactory& dutFactory,
                            const qa::scenario::ExecutionConfig& config,
                            qa::scenario::ScenarioResult& result)
{
    if (!config.enableProfiling || result.performanceMetrics != nullptr)
        return;

    if (result.status == qa::scenario::ScenarioResult::Status::ERROR
        || result.status == qa::scenario::ScenarioResult::Status::SKIP)
    {
        return;
    }

    auto dut = dutFactory ? dutFactory() : nullptr;
    if (!dut)
        return;

    qa::AudioConfig audioConfig;
    audioConfig.sampleRate = config.sampleRate;
    audioConfig.blockSize = config.blockSize;
    audioConfig.numChannels = config.numChannels;
    audioConfig.totalSamples = config.sampleRate;

    dut->prepare(audioConfig.sampleRate, audioConfig.blockSize, audioConfig.numChannels);
    applyScenarioParameters(*dut, mergedScenarioParameters(scenario));

    const auto metrics = qa::profileDspPerformance(*dut, audioConfig,
                                                   config.profilingIterations,
                                                   config.profilingWarmupIterations);
    dut->release();

    result.performanceMetrics = std::make_unique<qa::PerformanceMetrics>(metrics);
}

#if defined(QA_HOST_RUNNER_AVAILABLE)
std::string normalizedHostFormat(std::string format)
{
    std::transform(format.begin(), format.end(), format.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return format;
}

std::optional<qa::PluginFormat> parseHostFormat(const std::string& format)
{
    const std::string normalized = normalizedHostFormat(format);
    if (normalized == "vst3")
        return qa::PluginFormat::VST3;
    if (normalized == "au")
        return qa::PluginFormat::AU;
    return std::nullopt;
}

std::unique_ptr<qa::PluginHostInterface> createHostBackend(qa::PluginFormat format)
{
    switch (format)
    {
        case qa::PluginFormat::VST3:
            return std::make_unique<qa::VST3PluginHost>();
        case qa::PluginFormat::CLAP:
            return nullptr;
        case qa::PluginFormat::AU:
#if defined(__APPLE__)
            return std::make_unique<qa::AUPluginHost>();
#else
            return nullptr;
#endif
    }

    return nullptr;
}

qa::TestSpec createHostRunnerSmokeSpec(const std::filesystem::path& outputDir,
                                       const LocusQRunOptions& options)
{
    static qa::texture::PresetSpec preset;
    static qa::texture::StimulusSpec stimulus;
    static qa::texture::KeyScaleSpec key;
    static qa::texture::GlobalConfig globalConfig;

    globalConfig.sampleRateHz = options.sampleRate;
    globalConfig.blockSizeSamples = options.blockSize;
    globalConfig.renderChannels = options.numChannels;
    globalConfig.barsTotal = 1;
    globalConfig.bpm = 120;
    globalConfig.seed = 1337;

    stimulus.type = "Impulse";
    key.midiRoot = 60;
    key.scale = "major";

    qa::TestSpec spec;
    spec.preset = &preset;
    spec.stimulus = &stimulus;
    spec.key = &key;
    spec.globalConfig = &globalConfig;
    spec.outputDir = outputDir;
    return spec;
}

int runHostRunnerSmoke(const HostRunnerOptions& hostOptions, const LocusQRunOptions& runOptions)
{
    const auto parsedFormat = parseHostFormat(hostOptions.format);
    if (!parsedFormat.has_value())
    {
        std::cerr << "ERROR: --host-format must be one of: vst3, au\n";
        return 1;
    }

    const qa::PluginFormat format = *parsedFormat;
    const auto pluginPath = std::filesystem::path(hostOptions.pluginPath);
    if (!std::filesystem::exists(pluginPath))
    {
        std::cerr << "ERROR: --host-plugin path does not exist: " << pluginPath << "\n";
        return 1;
    }

    const std::filesystem::path outputDir = hostOptions.outputDir.empty()
        ? std::filesystem::path("qa_output/locusq_hostrunner_smoke")
        : std::filesystem::path(hostOptions.outputDir);

    std::filesystem::create_directories(outputDir);

    qa::HostConfig hostConfig;
    hostConfig.pluginPath = pluginPath;
    hostConfig.format = format;
    hostConfig.sampleRate = runOptions.sampleRate;
    hostConfig.blockSize = runOptions.blockSize;
    hostConfig.numChannels = runOptions.numChannels;

    qa::AudioConfig audioConfig;
    audioConfig.sampleRate = runOptions.sampleRate;
    audioConfig.blockSize = runOptions.blockSize;
    audioConfig.numChannels = runOptions.numChannels;
    audioConfig.totalSamples = runOptions.sampleRate;

    auto logStage = [&](const std::string& stage, const std::string& detail = std::string()) {
        std::cerr << "HOSTRUNNER_STAGE " << stage;
        if (!detail.empty())
            std::cerr << " detail=\"" << detail << "\"";
        std::cerr << "\n";
    };

    std::cerr << std::unitbuf;
    logStage("init",
             std::string("format=") + qa::pluginFormatToString(format)
                 + " plugin=" + pluginPath.string());

    auto runProbe = [&](qa::HostRunner& runner) -> int
    {
        logStage("prepare_begin");
        if (!runner.prepare(audioConfig))
        {
            logStage("prepare_failed");
            std::cerr << "ERROR: HostRunner prepare failed for " << pluginPath << "\n";
            return 1;
        }
        logStage("prepare_ok");

        const auto spec = createHostRunnerSmokeSpec(outputDir, runOptions);
        logStage("render_begin");
        const auto result = runner.renderTest(spec);
        logStage("render_done",
                 std::string("status=") + std::to_string(static_cast<int>(result.status))
                     + " error=" + result.errorMessage);
        logStage("release_begin");
        runner.release();
        logStage("release_done");

        if (hostOptions.skeletonMode)
        {
            if (result.status != qa::RenderResult::Status::SKIPPED)
            {
                std::cerr << "ERROR: HostRunner skeleton mode expected SKIPPED status, got "
                          << static_cast<int>(result.status) << "\n";
                return 1;
            }

            std::cout << "HOSTRUNNER_SKELETON_PASS format=" << qa::pluginFormatToString(format)
                      << " plugin=" << pluginPath
                      << " reason=" << result.errorMessage << "\n";
            return 0;
        }

        if (result.status != qa::RenderResult::Status::SUCCESS)
        {
            std::cerr << "ERROR: HostRunner render failed (status="
                      << static_cast<int>(result.status)
                      << ", error=" << result.errorMessage << ")\n";
            return 1;
        }

        if (!std::filesystem::exists(result.dryPath) || !std::filesystem::exists(result.wetPath))
        {
            std::cerr << "ERROR: HostRunner did not emit dry/wet output files\n";
            return 1;
        }

        std::cout << "HOSTRUNNER_SMOKE_PASS format=" << qa::pluginFormatToString(format)
                  << " plugin=" << pluginPath
                  << " dry=" << result.dryPath
                  << " wet=" << result.wetPath << "\n";
        return 0;
    };

    if (hostOptions.skeletonMode)
    {
        qa::HostRunner runner(hostConfig);
        return runProbe(runner);
    }

    qa::PluginHostFactory hostFactory = [format]() {
        return createHostBackend(format);
    };
    qa::HostRunner runner(hostConfig, hostFactory);
    return runProbe(runner);
}
#endif

class LocusQQARunner final : public qa::runner_app::BaseQARunner<LocusQRunOptions>
{
protected:
    std::filesystem::path smokeTestPath() override
    {
        return std::filesystem::path(__FILE__).parent_path() / "scenarios/locusq_smoke_suite.json";
    }

    qa::DutFactory createDutFactory(const LocusQRunOptions& options) override
    {
        if (options.useCalibrate)
            return createCalibrateDut;
        if (options.useSpatial)
            return createSpatialDut;
        return createEmitterDut;
    }

    std::filesystem::path outputRoot(const LocusQRunOptions& options) override
    {
        const char* suffix = options.useCalibrate ? "calibrate" : (options.useSpatial ? "spatial" : "emitter");
        return std::filesystem::path("qa_output") / ("locusq_" + std::string(suffix));
    }

    bool handleCustomOption(const std::vector<std::string>& args, size_t& index,
                            LocusQRunOptions& options, std::string&) override
    {
        const std::string& arg = args[index];

        auto consumeWithValue = [&](std::string& destination, const char* flag) {
            if (index + 1 >= args.size())
            {
                options.parseError = std::string(flag) + " requires an argument";
                index = args.size();
                return true;
            }

            destination = args[index + 1];
            index += 2;
            return true;
        };

        if (arg == "--spatial")
        {
            options.useSpatial = true;
            ++index;
            return true;
        }

        if (arg == "--calibrate")
        {
            options.useCalibrate = true;
            ++index;
            return true;
        }

        if (arg == "--host-runner-smoke")
        {
            options.hostRunner.enabled = true;
            ++index;
            return true;
        }

        if (arg == "--host-format")
            return consumeWithValue(options.hostRunner.format, "--host-format");

        if (arg == "--host-plugin")
            return consumeWithValue(options.hostRunner.pluginPath, "--host-plugin");

        if (arg == "--host-output")
            return consumeWithValue(options.hostRunner.outputDir, "--host-output");

        if (arg == "--host-skeleton")
        {
            options.hostRunner.skeletonMode = true;
            ++index;
            return true;
        }

        return false;
    }

    std::optional<int> maybeHandleCustomRun(const LocusQRunOptions& options) override
    {
        if (!options.parseError.empty())
        {
            std::cerr << "ERROR: " << options.parseError << "\n";
            return 1;
        }

        if (!options.hostRunner.enabled)
            return std::nullopt;

        if (options.hostRunner.format.empty() || options.hostRunner.pluginPath.empty())
        {
            std::cerr << "ERROR: --host-runner-smoke requires --host-format and --host-plugin\n";
            return 1;
        }

#if defined(QA_HOST_RUNNER_AVAILABLE)
        return runHostRunnerSmoke(options.hostRunner, options);
#else
        std::cerr << "ERROR: HostRunner support not available in this build. "
                  << "Reconfigure with -DBUILD_HOST_RUNNER=ON.\n";
        return 1;
#endif
    }

    void printCustomUsage(std::ostream& output) override
    {
        output << "  --spatial                       Use the LocusQ spatial adapter\n";
        output << "  --calibrate                     Use the LocusQ calibrate adapter\n";
        output << "  --host-runner-smoke             Run HostRunner smoke probe (requires BUILD_HOST_RUNNER=ON)\n";
        output << "  --host-format <fmt>             HostRunner format: vst3|au\n";
        output << "  --host-plugin <path>            Plugin path for HostRunner smoke probe\n";
        output << "  --host-output <dir>             Output directory for HostRunner smoke probe\n";
        output << "  --host-skeleton                 Run HostRunner without backend (expects SKIPPED)\n";
    }

    void afterScenarioExecution(const qa::scenario::ScenarioSpec& scenario,
                                qa::scenario::ScenarioResult& result,
                                const qa::scenario::ExecutionConfig& config,
                                const LocusQRunOptions& options) override
    {
        attachProfilingMetrics(scenario, createDutFactory(options), config, result);
    }
};

} // namespace

int runLocusQQA(int argc, char** argv)
{
    LocusQQARunner runner;
    return runner.run(argc, argv);
}
