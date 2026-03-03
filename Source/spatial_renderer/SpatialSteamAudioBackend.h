#pragma once

#include <atomic>
#include <cstdlib>

#include <juce_core/juce_core.h>

#include "SpatialRendererTypes.h"

namespace locusq::spatial_steam_backend
{

using SteamInitStage = spatial_renderer_types::SteamInitStage;

inline bool isSteamAudioBackendCompiled() noexcept
{
#if defined (LOCUSQ_ENABLE_STEAM_AUDIO) && LOCUSQ_ENABLE_STEAM_AUDIO
    return true;
#else
    return false;
#endif
}

inline const char* steamInitStageToString (int stageIndex) noexcept
{
    switch (static_cast<SteamInitStage> (stageIndex))
    {
        case SteamInitStage::NotCompiled: return "not_compiled";
        case SteamInitStage::Uninitialized: return "uninitialized";
        case SteamInitStage::LoadingLibrary: return "loading_library";
        case SteamInitStage::LibraryOpenFailed: return "library_open_failed";
        case SteamInitStage::ResolvingSymbols: return "resolving_symbols";
        case SteamInitStage::SymbolsMissing: return "symbols_missing";
        case SteamInitStage::CreatingContext: return "creating_context";
        case SteamInitStage::ContextCreateFailed: return "context_create_failed";
        case SteamInitStage::CreatingHRTF: return "creating_hrtf";
        case SteamInitStage::HRTFCreateFailed: return "hrtf_create_failed";
        case SteamInitStage::CreatingVirtualSurround: return "creating_virtual_surround";
        case SteamInitStage::VirtualSurroundCreateFailed: return "virtual_surround_create_failed";
        case SteamInitStage::Ready: return "ready";
        default: break;
    }

    return "unknown";
}

inline void setSteamInitStage (std::atomic<int>& stageIndex,
                               std::atomic<int>& errorCode,
                               SteamInitStage stage,
                               int error) noexcept
{
    errorCode.store (error, std::memory_order_relaxed);
    stageIndex.store (static_cast<int> (stage), std::memory_order_relaxed);
}

inline void clearSteamDiagnosticsStrings (juce::SpinLock& lock,
                                          juce::String& runtimeLibraryPath,
                                          juce::String& missingSymbolName)
{
    const juce::SpinLock::ScopedLockType diagnosticsLock (lock);
    runtimeLibraryPath.clear();
    missingSymbolName.clear();
}

inline void setSteamRuntimeLibraryPathForDiagnostics (juce::SpinLock& lock,
                                                      juce::String& runtimeLibraryPath,
                                                      const juce::String& libraryPath)
{
    const juce::SpinLock::ScopedLockType diagnosticsLock (lock);
    runtimeLibraryPath = libraryPath;
}

inline void setSteamMissingSymbolForDiagnostics (juce::SpinLock& lock,
                                                 juce::String& missingSymbolName,
                                                 const juce::String& symbolName)
{
    const juce::SpinLock::ScopedLockType diagnosticsLock (lock);
    missingSymbolName = symbolName;
}

inline juce::String defaultSteamRuntimeLibraryName()
{
#if JUCE_MAC
    return "libphonon.dylib";
#elif JUCE_WINDOWS
    return "phonon.dll";
#else
    return "libphonon.so";
#endif
}

inline void appendSteamRuntimeCandidates (juce::StringArray& runtimeCandidates,
                                          const juce::File& executableDir)
{
    if (const auto* envPath = std::getenv ("LOCUSQ_STEAM_AUDIO_LIB"))
    {
        const auto candidate = juce::String (envPath).trim();
        if (candidate.isNotEmpty())
            runtimeCandidates.add (candidate);
    }

    runtimeCandidates.add (executableDir.getChildFile (defaultSteamRuntimeLibraryName()).getFullPathName());

#if JUCE_MAC
    runtimeCandidates.add (executableDir.getParentDirectory()
        .getChildFile ("Frameworks")
        .getChildFile ("libphonon.dylib")
        .getFullPathName());
#endif

#if defined (LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH)
    const auto compiledDefaultPath = juce::String (LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH).trim();
    if (compiledDefaultPath.isNotEmpty())
        runtimeCandidates.add (compiledDefaultPath);
#endif

    runtimeCandidates.removeEmptyStrings();
    runtimeCandidates.removeDuplicates (false);
}

inline bool tryOpenSteamRuntimeLibrary (juce::DynamicLibrary& steamAudioLibrary,
                                        const juce::StringArray& runtimeCandidates,
                                        juce::String& attemptedLibraryPath,
                                        juce::String& loadedLibraryPath)
{
    attemptedLibraryPath.clear();
    loadedLibraryPath.clear();

    for (const auto& candidatePath : runtimeCandidates)
    {
        if (candidatePath.isEmpty())
            continue;

        attemptedLibraryPath = attemptedLibraryPath.isNotEmpty()
            ? attemptedLibraryPath + ";" + candidatePath
            : candidatePath;

        if (steamAudioLibrary.open (candidatePath))
        {
            loadedLibraryPath = candidatePath;
            return true;
        }
    }

    const auto fallbackLibraryName = defaultSteamRuntimeLibraryName();
    attemptedLibraryPath = attemptedLibraryPath.isNotEmpty()
                               ? attemptedLibraryPath + ";" + fallbackLibraryName
                               : fallbackLibraryName;
    if (steamAudioLibrary.open (fallbackLibraryName))
    {
        loadedLibraryPath = fallbackLibraryName;
        return true;
    }

    return false;
}

template <typename FnType>
inline bool resolveRequiredSymbol (juce::DynamicLibrary& steamAudioLibrary,
                                   const char* symbolName,
                                   FnType& fnOut) noexcept
{
    fnOut = reinterpret_cast<FnType> (steamAudioLibrary.getFunction (symbolName));
    return fnOut != nullptr;
}

} // namespace locusq::spatial_steam_backend
