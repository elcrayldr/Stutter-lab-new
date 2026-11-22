cmake_minimum_required(VERSION 3.22)
project(STUTTER_LAB VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Match the working CRAY AutoTune build: static MSVC runtime (/MT).
# This removes the VST3 dependency on the Visual C++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(STUTTER_LAB
    COMPANY_NAME "CRAY LDR"
    BUNDLE_ID "com.crayldr.stutterlab"
    PLUGIN_MANUFACTURER_CODE CrLd
    PLUGIN_CODE StLb
    FORMATS VST3
    PRODUCT_NAME "STUTTER LAB"
    VST3_CATEGORIES Fx
    DESCRIPTION "Capture-based vocal stutter processor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(STUTTER_LAB)

target_sources(STUTTER_LAB PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)
target_include_directories(STUTTER_LAB PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(STUTTER_LAB PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    target_compile_options(STUTTER_LAB PRIVATE /utf-8)
endif()

target_link_libraries(STUTTER_LAB PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

option(STUTTERLAB_BUILD_TESTS "Build the JUCE-free core tests" OFF)
if(STUTTERLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
