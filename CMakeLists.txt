# LocusQ - Quadraphonic 3D Spatial Audio Tool
# WebView-based UI with Three.js 3D visualization
# Cross-platform: Windows (VST3), macOS (VST3/AU), Linux (VST3/LV2)

cmake_minimum_required(VERSION 3.22)
project(LocusQ VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Reuse JUCE from sibling audio-plugin-coder checkout when building standalone.
if(NOT TARGET juce::juce_core)
    if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
        set(JUCE_DIR "${JUCE_DIR}")
    elseif(DEFINED ENV{JUCE_DIR} AND EXISTS "$ENV{JUCE_DIR}/CMakeLists.txt")
        set(JUCE_DIR "$ENV{JUCE_DIR}")
    else()
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../audio-plugin-coder/_tools/JUCE")
    endif()

    if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
        add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
    else()
        message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}")
    endif()
endif()

# Feature gates that affect platform format wiring must be declared
# before PLATFORM-SPECIFIC CONFIGURATION is evaluated.
option(LOCUSQ_ENABLE_AUV3 "Enable AUv3 format generation on Apple hosts" OFF)

# ============================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEBVIEW2 TRUE)
    set(NEEDS_WEB_BROWSER FALSE)
    set(WEBVIEW_BACKEND "WebView2")
elseif(APPLE)
    if(LOCUSQ_ENABLE_AUV3)
        set(PLUGIN_FORMATS VST3 AU AUv3 Standalone)
    else()
        set(PLUGIN_FORMATS VST3 AU Standalone)
    endif()
    set(NEEDS_WEBVIEW2 FALSE)
    set(NEEDS_WEB_BROWSER TRUE)
    set(WEBVIEW_BACKEND "WKWebView")
    set(AU_MAIN_TYPE kAudioUnitType_Effect)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(NEEDS_WEBVIEW2 FALSE)
    set(NEEDS_WEB_BROWSER TRUE)
    set(WEBVIEW_BACKEND "WebKitGTK")
    set(LV2_URI "https://github.com/noizefield/audio-plugin-coder/LocusQ")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "LocusQ: Building for ${PLUGIN_FORMATS}")
message(STATUS "LocusQ: WebView backend: ${WEBVIEW_BACKEND}")
message(STATUS "LocusQ: AUv3 format gate: ${LOCUSQ_ENABLE_AUV3}")

# ============================================
# UI DEBUG/POC MODE
# ============================================
option(LOCUSQ_UI_POC "Load focused WebView POC UI by default" OFF)
option(LOCUSQ_ENABLE_STEAM_AUDIO "Enable Steam Audio headphone backend integration hook" OFF)
option(LOCUSQ_ENABLE_CLAP "Enable CLAP target generation via clap-juce-extensions" OFF)
option(LOCUS_HEAD_TRACKING "Enable BL-017 head-tracking companion bridge receiver integration" OFF)
option(LOCUSQ_CLAP_FETCH "Fetch clap-juce-extensions from Git if local path is missing" ON)
option(LOCUSQ_ENABLE_SOFA "Enable libmysofa SOFA HRTF loader (requires network on first configure)" OFF)
set(LOCUSQ_STEAM_AUDIO_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/steam-audio/sdk/steamaudio"
    CACHE PATH "Path to Steam Audio SDK root (contains include/phonon.h and lib/)")
set(LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/clap-juce-extensions"
    CACHE PATH "Path to clap-juce-extensions root")
set(LOCUSQ_CLAP_JUCE_EXTENSIONS_TAG "02f91b7988298f7f1f05c706da16e1d9da852a87"
    CACHE STRING "Tag/commit to fetch for clap-juce-extensions")
set(LOCUSQ_CLAP_ID "com.noizefield.locusq"
    CACHE STRING "Stable CLAP plugin ID")
set(LOCUSQ_CLAP_FEATURES "audio-effect;spatial"
    CACHE STRING "Semicolon-separated CLAP feature list")
set(LOCUSQ_CLAP_MANUAL_URL "https://github.com/noizefield/audio-plugin-coder/LocusQ"
    CACHE STRING "CLAP manual URL")
set(LOCUSQ_CLAP_SUPPORT_URL "https://github.com/noizefield/audio-plugin-coder/LocusQ/issues"
    CACHE STRING "CLAP support URL")

set(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB "")
set(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED "")
set(LOCUSQ_CLAP_EXTENSIONS_READY FALSE)

if(LOCUSQ_ENABLE_CLAP)
    set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "Disable clap-juce-extensions example targets" FORCE)

    if(EXISTS "${LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR}/CMakeLists.txt")
        message(STATUS "LocusQ: using local clap-juce-extensions at ${LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR}")
        add_subdirectory("${LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR}"
                         "${CMAKE_CURRENT_BINARY_DIR}/clap-juce-extensions"
                         EXCLUDE_FROM_ALL)
        set(LOCUSQ_CLAP_EXTENSIONS_READY TRUE)
    elseif(LOCUSQ_CLAP_FETCH)
        include(FetchContent)
        message(STATUS "LocusQ: fetching clap-juce-extensions (${LOCUSQ_CLAP_JUCE_EXTENSIONS_TAG})")
        FetchContent_Declare(
            locusq_clap_juce_extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            GIT_TAG ${LOCUSQ_CLAP_JUCE_EXTENSIONS_TAG}
            GIT_SHALLOW TRUE
            GIT_SUBMODULES "clap-libs/clap;clap-libs/clap-helpers"
            GIT_SUBMODULES_RECURSE TRUE
        )
        FetchContent_MakeAvailable(locusq_clap_juce_extensions)
        set(LOCUSQ_CLAP_EXTENSIONS_READY TRUE)
    else()
        message(FATAL_ERROR
            "LOCUSQ_ENABLE_CLAP=ON but clap-juce-extensions was not found at "
            "${LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR}. Set LOCUSQ_CLAP_FETCH=ON or point "
            "LOCUSQ_CLAP_JUCE_EXTENSIONS_DIR to a checkout.")
    endif()
endif()

# ============================================
# SOFA HRTF LOADER (libmysofa via FetchContent)
# Enable with: cmake -DLOCUSQ_ENABLE_SOFA=ON ..
# ============================================
if(LOCUSQ_ENABLE_SOFA)
    include(FetchContent)

    # Suppress libmysofa's own tests and utilities to keep build lean.
    set(BUILD_TESTS OFF CACHE BOOL "Disable libmysofa tests" FORCE)

    FetchContent_Declare(
        mysofa
        GIT_REPOSITORY https://github.com/hoene/libmysofa.git
        GIT_TAG        v1.3.2
        GIT_SHALLOW    TRUE
    )

    # Scope BUILD_SHARED_LIBS to mysofa only: save the current value, force
    # static for mysofa's configure step, then restore.  This avoids poisoning
    # subsequent FetchContent dependencies with a global FORCE override.
    set(_locusq_prev_shared_libs ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(mysofa)
    set(BUILD_SHARED_LIBS ${_locusq_prev_shared_libs})

    message(STATUS "LocusQ: SOFA HRTF loader enabled (libmysofa v1.3.2 via FetchContent)")
endif()

# ============================================
# BINARY DATA (Web UI Resources)
# ============================================
# Production UI resources — always included.
set(LOCUSQ_WEBUI_WORKSPACE "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui")
set(LOCUSQ_WEBUI_BUNDLE "${LOCUSQ_WEBUI_WORKSPACE}/generated/index.js")
set(LOCUSQ_WEBUI_DEPS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/locusq_webui_deps.stamp")

set(LOCUSQ_WEBUI_SOURCES
    Source/ui/public/index.html
    ${LOCUSQ_WEBUI_BUNDLE}
    Source/ui/public/js/juce/index.js
    Source/ui/public/js/juce/check_native_interop.js
)

find_program(LOCUSQ_NPM_EXECUTABLE NAMES npm npm.cmd)
if(LOCUSQ_NPM_EXECUTABLE)
    add_custom_command(
        OUTPUT "${LOCUSQ_WEBUI_DEPS_STAMP}"
        COMMAND "${CMAKE_COMMAND}" -E env
                "NODE_ENV=development"
                "npm_config_production=false"
                "npm_config_include=dev"
                "${LOCUSQ_NPM_EXECUTABLE}" ci --include=dev
        COMMAND "${CMAKE_COMMAND}" -E touch "${LOCUSQ_WEBUI_DEPS_STAMP}"
        WORKING_DIRECTORY "${LOCUSQ_WEBUI_WORKSPACE}"
        DEPENDS
            "${LOCUSQ_WEBUI_WORKSPACE}/package.json"
            "${LOCUSQ_WEBUI_WORKSPACE}/package-lock.json"
        COMMENT "Installing LocusQ WebView UI dependencies"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${LOCUSQ_WEBUI_BUNDLE}"
        COMMAND "${LOCUSQ_NPM_EXECUTABLE}" run build
        WORKING_DIRECTORY "${LOCUSQ_WEBUI_WORKSPACE}"
        DEPENDS
            "${LOCUSQ_WEBUI_DEPS_STAMP}"
            "${LOCUSQ_WEBUI_WORKSPACE}/src/index.ts"
            "${LOCUSQ_WEBUI_WORKSPACE}/tsconfig.json"
            "${LOCUSQ_WEBUI_WORKSPACE}/vite.config.mts"
        COMMENT "Building LocusQ WebView UI bundle with Vite"
        VERBATIM
    )

    add_custom_target(locusq_webui_bundle
        DEPENDS "${LOCUSQ_WEBUI_BUNDLE}"
    )

    add_custom_target(locusq_webui_typecheck
        COMMAND "${LOCUSQ_NPM_EXECUTABLE}" run typecheck
        WORKING_DIRECTORY "${LOCUSQ_WEBUI_WORKSPACE}"
        COMMENT "Type-checking LocusQ WebView UI sources"
        VERBATIM
    )
elseif(NOT EXISTS "${LOCUSQ_WEBUI_BUNDLE}")
    message(FATAL_ERROR
        "LocusQ: npm was not found and the generated WebView bundle is missing at "
        "${LOCUSQ_WEBUI_BUNDLE}. Install npm to build the Web UI bundle.")
else()
    message(WARNING
        "LocusQ: npm not found; using the existing generated WebView bundle at "
        "${LOCUSQ_WEBUI_BUNDLE}")
endif()

# POC and incremental stage assets — only included when LOCUSQ_UI_POC is ON.
# Keeps release binaries lean by excluding ~24 unused HTML/JS files.
if(LOCUSQ_UI_POC)
    list(APPEND LOCUSQ_WEBUI_SOURCES
        Source/ui/public/poc/index_poc.html
        Source/ui/public/poc/js/poc_ui.js
        Source/ui/public/incremental/index_stage2.html
        Source/ui/public/incremental/js/stage2_ui.js
        Source/ui/public/incremental/index_stage3.html
        Source/ui/public/incremental/js/stage3_ui.js
        Source/ui/public/incremental/index_stage4.html
        Source/ui/public/incremental/js/stage4_ui.js
        Source/ui/public/incremental/index_stage5.html
        Source/ui/public/incremental/js/stage5_ui.js
        Source/ui/public/incremental/index_stage6.html
        Source/ui/public/incremental/js/stage6_ui.js
        Source/ui/public/incremental/index_stage7.html
        Source/ui/public/incremental/js/stage7_ui.js
        Source/ui/public/incremental/index_stage8.html
        Source/ui/public/incremental/js/stage8_ui.js
        Source/ui/public/incremental/index_stage9.html
        Source/ui/public/incremental/js/stage9_ui.js
        Source/ui/public/incremental/index_stage10.html
        Source/ui/public/incremental/js/stage10_ui.js
        Source/ui/public/incremental/index_stage11.html
        Source/ui/public/incremental/js/stage11_ui.js
        Source/ui/public/incremental/index_stage12.html
        Source/ui/public/incremental/js/stage12_ui.js
    )
endif()

juce_add_binary_data(LocusQ_WebUI
    SOURCES ${LOCUSQ_WEBUI_SOURCES}
)

if(TARGET locusq_webui_bundle)
    add_dependencies(LocusQ_WebUI locusq_webui_bundle)
endif()

# ============================================
# STANDALONE APP ICON (macOS)
# ============================================
set(LOCUSQ_PLUGIN_ICON_ARGS)
set(LOCUSQ_STANDALONE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Resources/LocusQ.png")
if(APPLE)
    if(EXISTS "${LOCUSQ_STANDALONE_ICON}")
        list(APPEND LOCUSQ_PLUGIN_ICON_ARGS ICON_BIG "${LOCUSQ_STANDALONE_ICON}")
    else()
        message(WARNING "LocusQ: standalone icon missing at ${LOCUSQ_STANDALONE_ICON}")
    endif()
endif()

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(LocusQ
    COMPANY_NAME "Noizefield"
    PLUGIN_MANUFACTURER_CODE Nfld
    PLUGIN_CODE LcQd
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "LocusQ"
    ${LOCUSQ_PLUGIN_ICON_ARGS}
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER ${NEEDS_WEB_BROWSER}
    VST3_CATEGORIES Fx Spatial
    AU_MAIN_TYPE ${AU_MAIN_TYPE}
    LV2URI ${LV2_URI}
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(LocusQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/processor_core/ProcessorParameterLayout.cpp
        Source/processor_core/ProcessorSceneRegistration.cpp
        Source/processor_core/ProcessorAuthoringHistory.cpp
        Source/processor_core/ProcessorPresetManager.cpp
        Source/processor_core/ProcessorCalibrationBridge.cpp
        Source/processor_core/ProcessorStateSerializer.cpp
        Source/PluginEditor.cpp
        Source/SceneGraph.h
        Source/VBAPPanner.h
        Source/DistanceAttenuator.h
        Source/AirAbsorption.h
        Source/SpatialRenderer.cpp
        Source/SpatialRenderer.h
        Source/spatial_renderer/SpatialAuditionControl.cpp
        Source/spatial_renderer/SpatialAuditionSupport.cpp
        Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
        Source/spatial_renderer/SpatialAuditionRender.cpp
        Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
        Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
        Source/spatial_renderer/SpatialOutputRoutingStage.cpp
        Source/spatial_renderer/SpatialSteamAudioBackend.cpp
        Source/DopplerProcessor.h
        Source/DirectivityFilter.h
        Source/SpreadProcessor.h
        Source/EarlyReflections.h
        Source/FDNReverb.h
        Source/HeadTrackingBridge.h
        Source/TestSignalGenerator.h
        Source/IRCapture.h
        Source/RoomAnalyzer.h
        Source/RoomProfileSerializer.h
        Source/CalibrationEngine.h
        Source/PhysicsEngine.h
        Source/KeyframeTimeline.cpp
        Source/KeyframeTimeline.h
)

# SofaHrtfLoader.h/.cpp are both isolated to the SOFA feature block.
# The header unconditionally includes mysofa.h (guarded by #ifdef LOCUSQ_ENABLE_SOFA
# inside the header itself), but keeping it out of the unconditional source list
# prevents unity-build tools from pulling it in when the feature is OFF.
if(LOCUSQ_ENABLE_SOFA)
    target_sources(LocusQ PRIVATE
        Source/dsp/SofaHrtfLoader.h
        Source/dsp/SofaHrtfLoader.cpp
    )
endif()

# ============================================
# INCLUDE PATHS
# ============================================
target_include_directories(LocusQ
    PRIVATE
        Source
)

if(LOCUSQ_CLAP_EXTENSIONS_READY)
    if(NOT COMMAND clap_juce_extensions_plugin)
        message(FATAL_ERROR "LocusQ: clap-juce-extensions loaded but clap_juce_extensions_plugin() is unavailable")
    endif()

    set(_locusq_clap_features ${LOCUSQ_CLAP_FEATURES})
    if("${_locusq_clap_features}" STREQUAL "")
        set(_locusq_clap_features audio-effect spatial)
    endif()

    clap_juce_extensions_plugin(
        TARGET LocusQ
        CLAP_ID "${LOCUSQ_CLAP_ID}"
        CLAP_FEATURES ${_locusq_clap_features}
        CLAP_MANUAL_URL "${LOCUSQ_CLAP_MANUAL_URL}"
        CLAP_SUPPORT_URL "${LOCUSQ_CLAP_SUPPORT_URL}"
    )
    message(STATUS "LocusQ: CLAP target enabled (LocusQ_CLAP)")
endif()

if(LOCUSQ_ENABLE_STEAM_AUDIO)
    if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
        target_include_directories(LocusQ PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
    else()
        message(FATAL_ERROR
            "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
            "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include. "
            "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
    endif()

    if(APPLE)
        set(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/lib/osx/libphonon.dylib")
    elseif(WIN32)
        set(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/lib/windows-x64/phonon.dll")
    elseif(UNIX)
        set(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/lib/linux-x64/libphonon.so")
    endif()

    if(LOCUSQ_STEAM_AUDIO_RUNTIME_LIB)
        file(TO_CMAKE_PATH "${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB}" LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        if(NOT EXISTS "${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB}")
            message(WARNING "Steam Audio runtime library path configured but file is missing: ${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB}")
        endif()
    endif()
endif()

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(LocusQ
    PRIVATE
        LocusQ_WebUI
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(LOCUSQ_ENABLE_SOFA)
    target_link_libraries(LocusQ PRIVATE mysofa)
    target_include_directories(LocusQ PRIVATE "${mysofa_SOURCE_DIR}/src")
endif()

# ============================================
# COMPILE DEFINITIONS (Platform-Specific)
# ============================================
target_compile_definitions(LocusQ
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
        LOCUSQ_UI_POC_DEFAULT=$<IF:$<BOOL:${LOCUSQ_UI_POC}>,1,0>
        LOCUSQ_ENABLE_AUV3=$<IF:$<BOOL:${LOCUSQ_ENABLE_AUV3}>,1,0>
        LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
        LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
        LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
        LOCUSQ_ENABLE_SOFA=$<IF:$<BOOL:${LOCUSQ_ENABLE_SOFA}>,1,0>
)

if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
    target_compile_definitions(LocusQ
        PUBLIC
            LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
    )
endif()

if(WIN32)
    target_compile_definitions(LocusQ
        PUBLIC
            JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()

if(APPLE)
    target_compile_definitions(LocusQ
        PUBLIC
            JUCE_USE_CURL=0
    )
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(LocusQ
        PUBLIC
            JUCE_USE_CURL=0
            JUCE_JACK=1
    )
endif()

# ============================================
# QA HARNESS TARGET (optional)
# cmake -DBUILD_LOCUSQ_QA=ON ..
# ============================================
option(BUILD_LOCUSQ_QA "Build LocusQ QA harness test executable" OFF)

if(BUILD_LOCUSQ_QA)
    # ── Locate the harness-owned QA integration module ───────────────────
    # Default: sibling repo at ../audio-dsp-qa-harness (relative to this file).
    # Override: cmake -DQA_HARNESS_DIR=/path/to/audio-dsp-qa-harness ..
    if(NOT DEFINED QA_HARNESS_DIR)
        set(QA_HARNESS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../audio-dsp-qa-harness"
            CACHE PATH "Path to audio-dsp-qa-harness root")
    endif()

    set(_qa_harness_module_dir "")
    foreach(_qa_harness_candidate
            "${QA_HARNESS_DIR}/cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/audio-dsp-qa-harness/cmake")
        if(EXISTS "${_qa_harness_candidate}/qa_harness_integration.cmake")
            set(_qa_harness_module_dir "${_qa_harness_candidate}")
            break()
        endif()
    endforeach()

    if(_qa_harness_module_dir)
        list(APPEND CMAKE_MODULE_PATH "${_qa_harness_module_dir}")
        message(STATUS "LocusQ QA: Using harness integration module at ${_qa_harness_module_dir}")
    else()
        find_package(audio_dsp_qa_harness REQUIRED
            PATHS "$ENV{HOME}/.local/lib/cmake/audio_dsp_qa_harness"
                  "/usr/local/lib/cmake/audio_dsp_qa_harness")
        if(DEFINED audio_dsp_qa_harness_CMAKE_DIR)
            list(APPEND CMAKE_MODULE_PATH "${audio_dsp_qa_harness_CMAKE_DIR}")
            message(STATUS "LocusQ QA: Using installed harness module at ${audio_dsp_qa_harness_CMAKE_DIR}")
        else()
            message(FATAL_ERROR
                "LocusQ QA: audio_dsp_qa_harness package was found but did not expose "
                "audio_dsp_qa_harness_CMAKE_DIR for qa_harness_integration.cmake")
        endif()
    endif()

    include(qa_harness_integration)

    # ── QA executable ────────────────────────────────────────────────────
    juce_add_console_app(locusq_qa PRODUCT_NAME "LocusQ QA Harness")
    juce_generate_juce_header(locusq_qa)

    target_sources(locusq_qa
        PRIVATE
            # Include processor sources directly (no plugin client)
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    enable_qa_harness(
        TARGET locusq_qa
        HARNESS_DIR "${QA_HARNESS_DIR}"
        ADAPTER_SOURCES
            qa/LocusQQARunner.h
            qa/LocusQQARunner.cpp
            qa/locusq_adapter.h
            qa/locusq_adapter.cpp
            qa/main.cpp
        EXTRA_INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
        QA_FLAG_NAME BUILD_LOCUSQ_QA
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_qa PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_qa. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_qa
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            # Plugin identity macros (required by JUCE plugin client without juce_add_plugin)
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_qa
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_qa
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_qa' created")
    message(STATUS "LocusQ QA: Scenarios in ${CMAKE_CURRENT_SOURCE_DIR}/qa/scenarios/")

    # ── BL-018 diagnostics probe (scene-state requested/active/stage capture) ──
    juce_add_console_app(locusq_bl018_profile_probe PRODUCT_NAME "LocusQ BL-018 Profile Probe")
    juce_generate_juce_header(locusq_bl018_profile_probe)

    target_sources(locusq_bl018_profile_probe
        PRIVATE
            qa/bl018_profile_probe_main.cpp
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    target_include_directories(locusq_bl018_profile_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_bl018_profile_probe PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_bl018_profile_probe. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_bl018_profile_probe
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_bl018_profile_probe
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_bl018_profile_probe
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_bl018_profile_probe' created")

    # ── Phase 2.4 physics acceptance probe ──────────────────────────────
    juce_add_console_app(locusq_physics_probe PRODUCT_NAME "LocusQ Physics Probe")
    juce_generate_juce_header(locusq_physics_probe)

    target_sources(locusq_physics_probe
        PRIVATE
            qa/physics_probe_main.cpp
            Source/SceneGraph.h
            Source/PhysicsEngine.h
    )

    target_include_directories(locusq_physics_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    target_compile_definitions(locusq_physics_probe
        PRIVATE
            LOCUSQ_TESTING=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
    )

    target_link_libraries(locusq_physics_probe
        PRIVATE
            juce::juce_core
            juce::juce_events
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_probe' created")

    # ── Physics runtime integration probe (shared worker -> scene spread) ─────
    juce_add_console_app(locusq_physics_runtime_attractor_probe PRODUCT_NAME "LocusQ Physics Runtime Attractor Probe")
    juce_generate_juce_header(locusq_physics_runtime_attractor_probe)

    target_sources(locusq_physics_runtime_attractor_probe
        PRIVATE
            qa/physics_runtime_attractor_probe_main.cpp
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    target_include_directories(locusq_physics_runtime_attractor_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_physics_runtime_attractor_probe PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_physics_runtime_attractor_probe. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_physics_runtime_attractor_probe
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_physics_runtime_attractor_probe
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_physics_runtime_attractor_probe
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_runtime_attractor_probe' created")

    # ── Physics runtime integration probe (worker-owned boundary response) ────
    juce_add_console_app(locusq_physics_runtime_boundary_probe PRODUCT_NAME "LocusQ Physics Runtime Boundary Probe")
    juce_generate_juce_header(locusq_physics_runtime_boundary_probe)

    target_sources(locusq_physics_runtime_boundary_probe
        PRIVATE
            qa/physics_runtime_boundary_probe_main.cpp
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    target_include_directories(locusq_physics_runtime_boundary_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_physics_runtime_boundary_probe PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_physics_runtime_boundary_probe. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_physics_runtime_boundary_probe
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_physics_runtime_boundary_probe
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_physics_runtime_boundary_probe
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_runtime_boundary_probe' created")

    # ── Physics runtime integration probe (shared-worker emitter collision) ───
    juce_add_console_app(locusq_physics_runtime_collision_probe PRODUCT_NAME "LocusQ Physics Runtime Collision Probe")
    juce_generate_juce_header(locusq_physics_runtime_collision_probe)

    target_sources(locusq_physics_runtime_collision_probe
        PRIVATE
            qa/physics_runtime_collision_probe_main.cpp
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    target_include_directories(locusq_physics_runtime_collision_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_physics_runtime_collision_probe PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_physics_runtime_collision_probe. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_physics_runtime_collision_probe
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_physics_runtime_collision_probe
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_physics_runtime_collision_probe
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_runtime_collision_probe' created")

    # ── Physics runtime integration probe (worker-owned boids response) ──────
    juce_add_console_app(locusq_physics_runtime_boids_probe PRODUCT_NAME "LocusQ Physics Runtime Boids Probe")
    juce_generate_juce_header(locusq_physics_runtime_boids_probe)

    target_sources(locusq_physics_runtime_boids_probe
        PRIVATE
            qa/physics_runtime_boids_probe_main.cpp
            Source/PluginProcessor.cpp
            Source/processor_core/ProcessorAuthoringHistory.cpp
            Source/processor_core/ProcessorParameterLayout.cpp
            Source/processor_core/ProcessorSceneRegistration.cpp
            Source/processor_core/ProcessorPresetManager.cpp
            Source/processor_core/ProcessorCalibrationBridge.cpp
            Source/processor_core/ProcessorStateSerializer.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.cpp
            Source/SpatialRenderer.h
            Source/spatial_renderer/SpatialAuditionControl.cpp
            Source/spatial_renderer/SpatialAuditionSupport.cpp
            Source/spatial_renderer/SpatialAuditionSignalGenerator.cpp
            Source/spatial_renderer/SpatialAuditionRender.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileControl.cpp
            Source/spatial_renderer/SpatialHeadphoneProfileSupport.cpp
            Source/spatial_renderer/SpatialOutputRoutingStage.cpp
            Source/spatial_renderer/SpatialSteamAudioBackend.cpp
            Source/DopplerProcessor.h
            Source/DirectivityFilter.h
            Source/SpreadProcessor.h
            Source/EarlyReflections.h
            Source/FDNReverb.h
            Source/HeadTrackingBridge.h
            Source/PhysicsEngine.h
            Source/KeyframeTimeline.cpp
            Source/KeyframeTimeline.h
    )

    target_include_directories(locusq_physics_runtime_boids_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_physics_runtime_boids_probe PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_physics_runtime_boids_probe. "
                "Install the pinned dependency with ./scripts/install-steam-audio-sdk.sh")
        endif()
    endif()

    target_compile_definitions(locusq_physics_runtime_boids_probe
        PRIVATE
            LOCUSQ_TESTING=1
            LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
            LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
            LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
            JucePlugin_Name="LocusQ"
            JucePlugin_Manufacturer="Noizefield"
            JucePlugin_ManufacturerCode=0x4e666c64
            JucePlugin_PluginCode=0x4c635164
            JucePlugin_IsSynth=0
            JucePlugin_WantsMidiInput=0
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JucePlugin_EditorRequiresKeyboardFocus=0
            JucePlugin_Version=0.0.0
            JucePlugin_VersionCode=0x00000
            JucePlugin_VersionString="0.0.0"
            JucePlugin_VSTUniqueID=JucePlugin_PluginCode
            JucePlugin_VSTCategory=kPlugCategEffect
            JucePlugin_Vst3Category="Fx|Spatial"
            JucePlugin_AUMainType=kAudioUnitType_Effect
            JucePlugin_AUSubType=JucePlugin_PluginCode
            JucePlugin_AUExportPrefix=LocusQAU
            JucePlugin_AUExportPrefixQuoted="LocusQAU"
            JucePlugin_AUManufacturerCode=JucePlugin_ManufacturerCode
            JucePlugin_CFBundleIdentifier=com.noizefield.LocusQ
            JucePlugin_MaxNumInputChannels=2
            JucePlugin_MaxNumOutputChannels=4
            JucePlugin_PreferredChannelConfigurations={1,1},{2,2}
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO AND LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED)
        target_compile_definitions(locusq_physics_runtime_boids_probe
            PRIVATE
                LOCUSQ_STEAM_AUDIO_DEFAULT_LIB_PATH="${LOCUSQ_STEAM_AUDIO_RUNTIME_LIB_ESCAPED}"
        )
    endif()

    target_link_libraries(locusq_physics_runtime_boids_probe
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_devices
            juce::juce_audio_formats
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_core
            juce::juce_data_structures
            juce::juce_dsp
            juce::juce_events
            juce::juce_graphics
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_runtime_boids_probe' created")

    # ── Physics Tier A acceptance probe (P1–P6 acceptance gates) ─────────────
    juce_add_console_app(locusq_physics_tier_a_probe PRODUCT_NAME "LocusQ Physics Tier A Probe")
    juce_generate_juce_header(locusq_physics_tier_a_probe)

    target_sources(locusq_physics_tier_a_probe
        PRIVATE
            qa/physics_tier_a_probe_main.cpp
            Source/SceneGraph.h
            Source/PhysicsEngine.h
            Source/PhysicsDSPBridge.h
            Source/PhysicsWorker.h
            Source/AttractorSystem.h
            Source/SpringSystem.h
            Source/TurbulenceSystem.h
            Source/AngularPhysicsSystem.h
            Source/BoidsSystem.h
            Source/CollisionSystem.h
    )

    target_include_directories(locusq_physics_tier_a_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    target_compile_definitions(locusq_physics_tier_a_probe
        PRIVATE
            LOCUSQ_TESTING=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
    )

    target_link_libraries(locusq_physics_tier_a_probe
        PRIVATE
            juce::juce_core
            juce::juce_events
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_tier_a_probe' created")

    # ── Physics DAW Automation acceptance probe (live/frozen path gates) ──────
    juce_add_console_app(locusq_physics_daw_automation_probe PRODUCT_NAME "LocusQ Physics DAW Automation Probe")
    juce_generate_juce_header(locusq_physics_daw_automation_probe)

    target_sources(locusq_physics_daw_automation_probe
        PRIVATE
            qa/physics_daw_automation_probe_main.cpp
            Source/SceneGraph.h
            Source/PhysicsDSPBridge.h
    )

    target_include_directories(locusq_physics_daw_automation_probe
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/qa
            Source
    )

    target_compile_definitions(locusq_physics_daw_automation_probe
        PRIVATE
            LOCUSQ_TESTING=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            _USE_MATH_DEFINES=1
    )

    target_link_libraries(locusq_physics_daw_automation_probe
        PRIVATE
            juce::juce_core
            juce::juce_events
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    message(STATUS "LocusQ QA: Target 'locusq_physics_daw_automation_probe' created")
endif()
