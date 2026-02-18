# LocusQ - Quadraphonic 3D Spatial Audio Tool
# JUCE 8 Plugin: VST3 / AU / LV2 / Standalone
# WebView UI with Three.js 3D visualization

cmake_minimum_required(VERSION 3.22)
project(LocusQ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# PLATFORM DETECTION
# ============================================
if(WIN32)
    set(LOCUSQ_PLATFORM "Windows")
elseif(APPLE)
    set(LOCUSQ_PLATFORM "macOS")
elseif(UNIX)
    set(LOCUSQ_PLATFORM "Linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "=== LocusQ Build System ===")
message(STATUS "Platform: ${LOCUSQ_PLATFORM}")

# ============================================
# JUCE (via submodule or FetchContent)
# ============================================
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_tools/JUCE")

if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(STATUS "JUCE: Using local submodule at ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}")
else()
    message(STATUS "JUCE: Fetching via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
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
    set(NEEDS_WEB_BROWSER FALSE)
    set(WEBVIEW_BACKEND "WKWebView")
    set(AU_MAIN_TYPE kAudioUnitType_Effect)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(NEEDS_WEBVIEW2 FALSE)
    set(NEEDS_WEB_BROWSER TRUE)
    set(WEBVIEW_BACKEND "WebKitGTK")
    set(LV2_URI "https://github.com/joshband/LocusQ")
endif()

message(STATUS "Formats: ${PLUGIN_FORMATS}")
message(STATUS "WebView backend: ${WEBVIEW_BACKEND}")

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
)

# ============================================
# INCLUDE PATHS
# ============================================
target_include_directories(LocusQ
    PRIVATE
        Source
)

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
)

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

message(STATUS "=== LocusQ configured ===")
