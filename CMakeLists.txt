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

# ============================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEBVIEW2 TRUE)
    set(NEEDS_WEB_BROWSER FALSE)
    set(WEBVIEW_BACKEND "WebView2")
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
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

# ============================================
# UI DEBUG/POC MODE
# ============================================
option(LOCUSQ_UI_POC "Load focused WebView POC UI by default" OFF)
option(LOCUSQ_ENABLE_STEAM_AUDIO "Enable Steam Audio headphone backend integration hook" OFF)
option(LOCUSQ_ENABLE_CLAP "Enable CLAP target generation via clap-juce-extensions" OFF)
option(LOCUS_HEAD_TRACKING "Enable BL-017 head-tracking companion bridge receiver integration" OFF)
option(LOCUSQ_CLAP_FETCH "Fetch clap-juce-extensions from Git if local path is missing" ON)
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
# BINARY DATA (Web UI Resources)
# ============================================
juce_add_binary_data(LocusQ_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/three.min.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
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

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(LocusQ
    COMPANY_NAME "Noizefield"
    PLUGIN_MANUFACTURER_CODE Nfld
    PLUGIN_CODE LcQd
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "LocusQ"
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
        Source/PluginEditor.cpp
        Source/SceneGraph.h
        Source/VBAPPanner.h
        Source/DistanceAttenuator.h
        Source/AirAbsorption.h
        Source/SpatialRenderer.h
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
        message(FATAL_ERROR "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at ${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
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

# ============================================
# COMPILE DEFINITIONS (Platform-Specific)
# ============================================
target_compile_definitions(LocusQ
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
        LOCUSQ_UI_POC_DEFAULT=$<IF:$<BOOL:${LOCUSQ_UI_POC}>,1,0>
        LOCUSQ_ENABLE_CLAP=$<IF:$<BOOL:${LOCUSQ_ENABLE_CLAP}>,1,0>
        LOCUSQ_ENABLE_STEAM_AUDIO=$<IF:$<BOOL:${LOCUSQ_ENABLE_STEAM_AUDIO}>,1,0>
        LOCUS_HEAD_TRACKING=$<IF:$<BOOL:${LOCUS_HEAD_TRACKING}>,1,0>
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
    # ── Locate the audio-dsp-qa-harness ──────────────────────────────────
    # Default: sibling repo at ../../audio-dsp-qa-harness (relative to this file).
    # Override: cmake -DQA_HARNESS_DIR=/path/to/audio-dsp-qa-harness ..
    if(NOT DEFINED QA_HARNESS_DIR)
        set(QA_HARNESS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../audio-dsp-qa-harness"
            CACHE PATH "Path to audio-dsp-qa-harness root")
    endif()

    if(EXISTS "${QA_HARNESS_DIR}/CMakeLists.txt")
        if(NOT TARGET qa_core)
            set(BUILD_QA_TESTS     OFF CACHE BOOL "Disable harness unit tests" FORCE)
            set(BUILD_QA_EXAMPLES  OFF CACHE BOOL "Disable harness examples"   FORCE)
            add_subdirectory("${QA_HARNESS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/qa_harness" EXCLUDE_FROM_ALL)
        endif()
        message(STATUS "LocusQ QA: Using harness at ${QA_HARNESS_DIR}")
    else()
        # Fall back to installed package
        find_package(audio_dsp_qa_harness REQUIRED
            PATHS "$ENV{HOME}/.local/lib/cmake/audio_dsp_qa_harness"
                  "/usr/local/lib/cmake/audio_dsp_qa_harness")
        message(STATUS "LocusQ QA: Using installed harness package")
    endif()

    # ── Resolve QA targets (support both namespaced and plain) ────────────
    if(TARGET qa_core)
        set(_qa_core     qa_core)
        set(_qa_runners  qa_runners)
        set(_qa_scenario qa_scenario_engine)
    elseif(TARGET qa::qa_core)
        set(_qa_core     qa::qa_core)
        set(_qa_runners  qa::qa_runners)
        set(_qa_scenario qa::qa_scenario_engine)
    else()
        message(FATAL_ERROR "LocusQ QA: qa_core target not found — check QA_HARNESS_DIR")
    endif()

    # ── QA executable ────────────────────────────────────────────────────
    juce_add_console_app(locusq_qa PRODUCT_NAME "LocusQ QA Harness")
    juce_generate_juce_header(locusq_qa)

    target_sources(locusq_qa
        PRIVATE
            qa/locusq_adapter.h
            qa/locusq_adapter.cpp
            qa/main.cpp
            # Include processor sources directly (no plugin client)
            Source/PluginProcessor.cpp
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.h
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

    target_include_directories(locusq_qa
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}          # for Source/ includes
            ${CMAKE_CURRENT_SOURCE_DIR}/qa        # for locusq_adapter.h
            "${QA_HARNESS_DIR}"                   # for core/, runners/, scenario_engine/
    )

    if(LOCUSQ_ENABLE_STEAM_AUDIO)
        if(EXISTS "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include/phonon.h")
            target_include_directories(locusq_qa PRIVATE "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include")
        else()
            message(FATAL_ERROR
                "LOCUSQ_ENABLE_STEAM_AUDIO=ON but phonon.h was not found at "
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_qa")
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
            ${_qa_core}
            ${_qa_runners}
            ${_qa_scenario}
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
            Source/SceneGraph.h
            Source/VBAPPanner.h
            Source/DistanceAttenuator.h
            Source/AirAbsorption.h
            Source/SpatialRenderer.h
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
                "${LOCUSQ_STEAM_AUDIO_SDK_ROOT}/include for locusq_bl018_profile_probe")
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
endif()
