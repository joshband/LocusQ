// LocusQ QA Harness - main.cpp
//
// Test runner for LocusQ DSP validation.
// Usage:
//   locusq_qa                          → smoke test suite
//   locusq_qa <scenario.json>          → run single scenario
//   locusq_qa <suite.json>             → run test suite
//   locusq_qa --spatial <scenario>     → run with LocusQSpatialAdapter
//   locusq_qa --discover <dir>         → auto-discover scenarios
//   locusq_qa --profile                → force perf profiling on all scenarios

#include "locusq_adapter.h"
#include "core/qa_runner.h"
#include "scenario_engine/scenario_executor.h"
#include "scenario_engine/scenario_loader.h"
#include "scenario_engine/test_suite_loader.h"
#include "scenario_engine/invariant_evaluator.h"
#include "scenario_engine/result_exporter.h"
#include "runners/in_process_runner.h"
#include "runners/performance_profiler.h"

#include <juce_events/juce_events.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

//------------------------------------------------------------------------------
// DUT factories

std::unique_ptr<qa::DspUnderTest> createEmitterDut()
{
    return std::make_unique<locusq::qa::LocusQEmitterAdapter>();
}

std::unique_ptr<qa::DspUnderTest> createSpatialDut()
{
    return std::make_unique<locusq::qa::LocusQSpatialAdapter>();
}

//------------------------------------------------------------------------------
// Runner factory helper

qa::scenario::QARunnerFactory makeRunnerFactory(qa::DutFactory dutFactory)
{
    return [defaultFactory = std::move(dutFactory)](qa::DutFactory effectiveFactory) mutable
        -> std::unique_ptr<qa::QARunner> {
        if (!effectiveFactory)
            effectiveFactory = defaultFactory;
        return std::make_unique<qa::InProcessRunner>(effectiveFactory);
    };
}

//------------------------------------------------------------------------------
// Base execution config

struct RunOptions
{
    bool forceProfiling = false;
    int profilingIterations = 1000;
    int profilingWarmupIterations = 10;
    int sampleRate = 48000;
    int blockSize = 512;
    int numChannels = 2;
};

qa::scenario::ExecutionConfig makeConfig(bool useSpatial, const RunOptions& options)
{
    qa::scenario::ExecutionConfig cfg;
    cfg.sampleRate   = options.sampleRate;
    cfg.blockSize    = options.blockSize;
    cfg.numChannels  = options.numChannels;
    cfg.outputDir    = "qa_output/locusq" + std::string(useSpatial ? "_spatial" : "_emitter");
    return cfg;
}

bool scenarioRequestsPerfMetrics(const qa::scenario::ScenarioSpec& scenario)
{
    if (!scenario.expectedInvariants.is_object())
        return false;

    for (auto it = scenario.expectedInvariants.begin(); it != scenario.expectedInvariants.end(); ++it)
    {
        const auto& spec = it.value();
        if (!spec.is_object())
            continue;

        const auto metric = spec.value("metric", std::string{});
        if (metric.rfind("perf_", 0) == 0)
            return true;
    }

    return false;
}

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
                          int paramCount)
{
    try
    {
        std::size_t parsed = 0;
        const int idx = std::stoi(key, &parsed);
        if (parsed == key.size() && idx >= 0 && idx < paramCount)
            return idx;
    }
    catch (const std::exception&)
    {
        // Key is not a numeric index, fall through to name lookup.
    }

    for (int i = 0; i < paramCount; ++i)
    {
        if (const auto* name = dut.getParameterName(i); name != nullptr && key == name)
            return i;
    }

    return -1;
}

void applyScenarioParameters(qa::DspUnderTest& dut, const nlohmann::json& params)
{
    if (!params.is_object())
        return;

    const int paramCount = dut.getParameterCount();

    for (auto it = params.begin(); it != params.end(); ++it)
    {
        if (!it.value().is_number())
            continue;

        const int idx = resolveParameterIndex(dut, it.key(), paramCount);
        if (idx < 0)
            continue;

        const float normalized = std::clamp(it.value().get<float>(), 0.0f, 1.0f);
        dut.setParameter(idx, normalized);
    }
}

void maybeProfileScenario(const qa::scenario::ScenarioSpec& scenario,
                          const qa::DutFactory& dutFactory,
                          const qa::scenario::ExecutionConfig& cfg,
                          const RunOptions& options,
                          qa::scenario::ScenarioResult& result)
{
    if (result.status == qa::scenario::ScenarioResult::Status::ERROR
        || result.status == qa::scenario::ScenarioResult::Status::SKIP)
    {
        return;
    }

    if (!options.forceProfiling && !scenarioRequestsPerfMetrics(scenario))
        return;

    qa::AudioConfig audioCfg;
    audioCfg.sampleRate = cfg.sampleRate;
    audioCfg.blockSize = cfg.blockSize;
    audioCfg.numChannels = cfg.numChannels;
    audioCfg.totalSamples = cfg.sampleRate;

    auto dut = dutFactory();
    if (!dut)
        return;

    dut->prepare(audioCfg.sampleRate, audioCfg.blockSize, audioCfg.numChannels);
    applyScenarioParameters(*dut, mergedScenarioParameters(scenario));

    auto metrics = qa::profileDspPerformance(*dut, audioCfg,
                                             options.profilingIterations,
                                             options.profilingWarmupIterations);
    dut->release();

    result.performanceMetrics = std::make_unique<qa::PerformanceMetrics>(metrics);
}

std::filesystem::path scenarioOutputDir(const qa::scenario::ScenarioResult& result,
                                        const qa::scenario::ExecutionConfig& cfg)
{
    if (!result.wetPath.empty())
        return result.wetPath.parent_path();
    if (!result.dryPath.empty())
        return result.dryPath.parent_path();
    if (!cfg.outputDir.empty() && !result.scenarioId.empty())
        return cfg.outputDir / result.scenarioId;
    return {};
}

//------------------------------------------------------------------------------
// Reporting helpers

std::string statusLabel(const qa::scenario::TestSuiteResult& r)
{
    if (r.errorCount > 0) return "ERROR";
    if (r.failCount  > 0) return "FAIL";
    if (r.warnCount  > 0) return "WARN";
    if (r.totalScenarios > 0 && r.skipCount == r.totalScenarios) return "SKIP";
    return "PASS";
}

void printSuiteSummary(const qa::scenario::TestSuiteResult& r)
{
    std::cout << "\n=== Suite: " << r.suiteId << " [" << statusLabel(r) << "] ===\n";
    std::cout << "  Total: " << r.totalScenarios
              << "  Pass: " << r.passCount
              << "  Warn: " << r.warnCount
              << "  Fail: " << r.failCount
              << "  Error: " << r.errorCount
              << "  Skip: " << r.skipCount << "\n";
}

int runSingleScenario(const std::string& path, bool useSpatial, const RunOptions& options)
{
    std::cout << "Running scenario: " << path
              << " [" << (useSpatial ? "Spatial" : "Emitter") << " adapter]\n";

    auto loadResult = qa::scenario::loadScenarioFile(path);
    if (!loadResult.ok)
    {
        std::cerr << "ERROR: Failed to load scenario\n";
        for (const auto& e : loadResult.errors)
            std::cerr << "  - " << e << "\n";
        return 1;
    }

    auto dutFactory = useSpatial ? createSpatialDut : createEmitterDut;
    auto cfg = makeConfig(useSpatial, options);

    qa::scenario::ScenarioExecutor executor(makeRunnerFactory(dutFactory), dutFactory, cfg);
    qa::scenario::ScenarioResult result = executor.execute(loadResult.scenario);
    maybeProfileScenario(loadResult.scenario, dutFactory, cfg, options, result);

    qa::scenario::InvariantEvaluator evaluator;
    evaluator.evaluateInto(loadResult.scenario, result);

    qa::scenario::ResultExporter exporter;
    const auto outDir = scenarioOutputDir(result, cfg);
    if (!outDir.empty())
    {
        try
        {
            exporter.exportResult(result, outDir);
        }
        catch (const std::exception& e)
        {
            std::cerr << "WARN: result export failed: " << e.what() << "\n";
        }
    }

    std::cout << "\nStatus: ";
    switch (result.status)
    {
        case qa::scenario::ScenarioResult::Status::PASS:   std::cout << "PASS\n"; break;
        case qa::scenario::ScenarioResult::Status::WARN:   std::cout << "WARN\n"; break;
        case qa::scenario::ScenarioResult::Status::FAIL:   std::cout << "FAIL\n"; break;
        case qa::scenario::ScenarioResult::Status::SKIP:   std::cout << "SKIP (" << result.skipReason << ")\n"; break;
        case qa::scenario::ScenarioResult::Status::ERROR:  std::cout << "ERROR (" << result.errorMessage << ")\n"; break;
    }

    for (const auto& inv : result.invariantResults)
    {
        std::cout << "  " << inv.metric << ": "
                  << (inv.passed ? "PASS" : "FAIL")
                  << " (value=" << inv.measuredValue << ")\n";
    }

    if (!result.hardFailures.empty())
    {
        std::cout << "\nHard Failures:\n";
        for (const auto& f : result.hardFailures)
            std::cout << "  - " << f << "\n";
    }

    return (result.status == qa::scenario::ScenarioResult::Status::PASS ||
            result.status == qa::scenario::ScenarioResult::Status::WARN) ? 0 : 1;
}

qa::scenario::TestSuiteResult executeSuite(const qa::scenario::TestSuite& suite,
                                           const std::vector<qa::scenario::ScenarioSpec>& scenarios,
                                           bool useSpatial,
                                           const RunOptions& options)
{
    auto dutFactory = useSpatial ? createSpatialDut : createEmitterDut;
    auto cfg = qa::scenario::applySuiteRuntimeConfig(makeConfig(useSpatial, options), suite);

    qa::scenario::ScenarioExecutor executor(makeRunnerFactory(dutFactory), dutFactory, cfg);
    qa::scenario::InvariantEvaluator evaluator;
    qa::scenario::ResultExporter exporter;

    qa::scenario::TestSuiteResult suiteResult;
    suiteResult.suiteId = suite.id;
    suiteResult.totalScenarios = static_cast<int>(scenarios.size());

    for (const auto& scenario : scenarios)
    {
        qa::scenario::ScenarioResult result = executor.execute(scenario);
        maybeProfileScenario(scenario, dutFactory, cfg, options, result);
        evaluator.evaluateInto(scenario, result);

        const auto outDir = scenarioOutputDir(result, cfg);
        if (!outDir.empty())
        {
            try
            {
                exporter.exportResult(result, outDir);
            }
            catch (const std::exception& e)
            {
                std::cerr << "WARN: result export failed for " << scenario.id
                          << ": " << e.what() << "\n";
            }
        }

        switch (result.status)
        {
            case qa::scenario::ScenarioResult::Status::PASS:  ++suiteResult.passCount;  break;
            case qa::scenario::ScenarioResult::Status::WARN:  ++suiteResult.warnCount;  break;
            case qa::scenario::ScenarioResult::Status::FAIL:  ++suiteResult.failCount;  break;
            case qa::scenario::ScenarioResult::Status::SKIP:  ++suiteResult.skipCount;  break;
            case qa::scenario::ScenarioResult::Status::ERROR: ++suiteResult.errorCount; break;
        }

        suiteResult.scenarioResults.push_back(std::move(result));

        if (suite.stopOnFirstFailure)
        {
            const auto& last = suiteResult.scenarioResults.back();
            if (last.status == qa::scenario::ScenarioResult::Status::FAIL
                || last.status == qa::scenario::ScenarioResult::Status::ERROR)
            {
                suiteResult.stoppedEarly = true;
                break;
            }
        }
    }

    suiteResult.passed = (suiteResult.failCount == 0 && suiteResult.errorCount == 0);

    if (!cfg.outputDir.empty())
    {
        try
        {
            exporter.exportSuiteResult(suiteResult, cfg.outputDir);
        }
        catch (const std::exception& e)
        {
            std::cerr << "WARN: suite result export failed: " << e.what() << "\n";
        }
    }

    return suiteResult;
}

int runSuite(const std::string& path, bool useSpatial, const RunOptions& options)
{
    std::cout << "Running suite: " << path << "\n";

    auto scenarioDir = std::filesystem::path(path).parent_path();
    auto resolved    = qa::scenario::loadAndResolveTestSuite(path, scenarioDir);

    if (!resolved.ok)
    {
        std::cerr << "ERROR: Failed to load suite\n";
        for (const auto& e : resolved.errors) std::cerr << "  - " << e << "\n";
        return 1;
    }

    auto result = executeSuite(resolved.suite, resolved.scenarios, useSpatial, options);
    printSuiteSummary(result);

    return result.passed ? 0 : 1;
}

void printUsage(const char* prog)
{
    std::cout << "Usage:\n"
              << "  " << prog << "                          Run smoke test suite\n"
              << "  " << prog << " <scenario.json>          Run single scenario (Emitter adapter)\n"
              << "  " << prog << " <suite.json>             Run test suite (Emitter adapter)\n"
              << "  " << prog << " --spatial <path>         Run scenario/suite with Spatial adapter\n"
              << "  " << prog << " --discover <dir>         Auto-discover scenarios in directory\n"
              << "  " << prog << " --profile                Force profiling for all scenarios\n"
              << "  " << prog << " --profile-iterations N   Override profiling iterations (default 1000)\n"
              << "  " << prog << " --profile-warmup N       Override profiling warmup iterations (default 10)\n"
              << "  " << prog << " --sample-rate N          Override runtime sample rate (default 48000)\n"
              << "  " << prog << " --block-size N           Override runtime block size (default 512)\n"
              << "  " << prog << " --channels N             Override runtime channel count (default 2)\n"
              << "  " << prog << " --help                   Show this help\n";
}

} // namespace

int main(int argc, char** argv)
{
    // Initialize JUCE message manager (required for parameter notifications)
    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        bool useSpatial = false;
        std::string inputPath;
        bool discoverMode = false;
        std::string discoverDir;
        RunOptions runOptions;

        for (int i = 1; i < argc; )
        {
            std::string arg(argv[i]);

            if (arg == "--help" || arg == "-h")
            {
                printUsage(argv[0]);
                return 0;
            }
            else if (arg == "--spatial")
            {
                useSpatial = true;
                ++i;
            }
            else if (arg == "--discover")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --discover requires a directory\n";
                    return 1;
                }
                discoverMode = true;
                discoverDir  = argv[i + 1];
                i += 2;
            }
            else if (arg == "--profile")
            {
                runOptions.forceProfiling = true;
                ++i;
            }
            else if (arg == "--profile-iterations")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --profile-iterations requires an integer\n";
                    return 1;
                }
                runOptions.profilingIterations = std::max(1, std::stoi(argv[i + 1]));
                i += 2;
            }
            else if (arg == "--profile-warmup")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --profile-warmup requires an integer\n";
                    return 1;
                }
                runOptions.profilingWarmupIterations = std::max(0, std::stoi(argv[i + 1]));
                i += 2;
            }
            else if (arg == "--sample-rate")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --sample-rate requires an integer\n";
                    return 1;
                }
                runOptions.sampleRate = std::max(1, std::stoi(argv[i + 1]));
                i += 2;
            }
            else if (arg == "--block-size")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --block-size requires an integer\n";
                    return 1;
                }
                runOptions.blockSize = std::max(1, std::stoi(argv[i + 1]));
                i += 2;
            }
            else if (arg == "--channels")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "ERROR: --channels requires an integer\n";
                    return 1;
                }
                runOptions.numChannels = std::max(1, std::stoi(argv[i + 1]));
                i += 2;
            }
            else if (arg[0] == '-')
            {
                std::cerr << "ERROR: Unknown option: " << arg << "\n";
                return 1;
            }
            else
            {
                inputPath = arg;
                ++i;
            }
        }

        if (discoverMode)
        {
            auto resolved = qa::scenario::discoverSuite(discoverDir, "", "");
            if (!resolved.ok)
            {
                for (const auto& e : resolved.errors) std::cerr << e << "\n";
                return 1;
            }
            std::cout << "Discovered " << resolved.scenarios.size() << " scenarios\n";

            auto result = executeSuite(resolved.suite, resolved.scenarios, useSpatial, runOptions);
            printSuiteSummary(result);
            return result.passed ? 0 : 1;
        }
        else if (!inputPath.empty())
        {
            // Detect suite vs scenario by filename convention
            bool isSuite = (inputPath.find("suite") != std::string::npos);
            return isSuite ? runSuite(inputPath, useSpatial, runOptions)
                           : runSingleScenario(inputPath, useSpatial, runOptions);
        }
        else
        {
            // Default: run smoke suite
            const std::string defaultSuite =
                std::filesystem::path(__FILE__).parent_path() / "scenarios/locusq_smoke_suite.json";
            std::cout << "Running default smoke suite\n";
            return runSuite(defaultSuite, false, runOptions);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }
}
