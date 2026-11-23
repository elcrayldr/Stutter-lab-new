cmake_minimum_required(VERSION 3.22)
project(STUTTER_LAB VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# JUCE 8.0.6 Fruity Wrapper state-stream compatibility workaround.
set(CRAY_JUCE_VST3_CLIENT "${JUCE_SOURCE_DIR}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")
file(READ "${CRAY_JUCE_VST3_CLIENT}" CRAY_JUCE_VST3_SOURCE)
set(CRAY_JUCE_OLD_READ_BLOCK [==[                auto status = state->read (buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);

                if (bytesRead <= 0 || (status != kResultTrue && ! detail::PluginUtilities::getHostType().isWavelab()))
                    break;

                allData.write (buffer, static_cast<size_t> (bytesRead));]==])
set(CRAY_JUCE_NEW_READ_BLOCK [==[                state->read (buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);

                // Some hosts may return kResultOk with a valid partial read.
                // Continue until the stream reports that no bytes remain.
                if (bytesRead <= 0)
                    break;

                allData.write (buffer, static_cast<size_t> (bytesRead));]==])
string(FIND "${CRAY_JUCE_VST3_SOURCE}" "${CRAY_JUCE_OLD_READ_BLOCK}" CRAY_JUCE_READ_BLOCK_POSITION)
if(CRAY_JUCE_READ_BLOCK_POSITION EQUAL -1)
    message(FATAL_ERROR "Could not apply CRAY LDR JUCE VST3 stream compatibility patch")
endif()
string(REPLACE "${CRAY_JUCE_OLD_READ_BLOCK}" "${CRAY_JUCE_NEW_READ_BLOCK}" CRAY_JUCE_VST3_SOURCE "${CRAY_JUCE_VST3_SOURCE}")
file(WRITE "${CRAY_JUCE_VST3_CLIENT}" "${CRAY_JUCE_VST3_SOURCE}")


juce_add_plugin(STUTTER_LAB
    COMPANY_NAME "Cray LDR"
    BUNDLE_ID "com.crayldr.stutterlab.v2"
    PLUGIN_MANUFACTURER_CODE CrLd
    PLUGIN_CODE StL2
    FORMATS VST3
    PRODUCT_NAME "CRAY LDR STUTTER LAB V2"
    VST3_CATEGORIES Fx
    DESCRIPTION "Capture-based vocal stutter processor - independent V2 identity"
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
    juce::juce_recommended_warning_flags
)
option(STUTTERLAB_BUILD_TESTS "Build the JUCE-free core tests" OFF)
if(STUTTERLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
