cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
# One universal binary loads in Apple Silicon hosts, Intel Macs and hosts running under Rosetta.
# Must be set before project(). For faster local iteration, configure with -DCMAKE_OSX_ARCHITECTURES=arm64.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures to build")

# Link the MSVC runtime statically (/MT) so users never hit "VCRUNTIME140.dll was not found" on Windows.
# Set for every target, including fetched dependencies, so the runtimes can't mismatch at link time.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Zither VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources are UTF-8. Without this MSVC reads them in the system code page and warns about non-ASCII comments.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Installing into ~/Library/Audio/Plug-Ins needs no special rights on macOS. On Windows the VST3 folder is
# under Program Files, which needs admin, so a normal build would fail there; install by hand or with an installer.
if(APPLE)
    set(ZITHER_COPY_PLUGIN_AFTER_BUILD TRUE)
else()
    set(ZITHER_COPY_PLUGIN_AFTER_BUILD FALSE)
endif()

option(ZITHER_BUILD_TESTS "Build the DSP unit tests" ON)

include(FetchContent)

FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.16-security
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(juce)

# Pure C++ DSP core (strings, exciters, tuning, strummer). No JUCE dependency, so the
# unit tests build and run quickly without pulling in the plugin.
add_library(zither_core STATIC
    Source/dsp/WaveguideString.cpp
    Source/dsp/Exciter.cpp
    Source/dsp/StringBank.cpp
    Source/tuning/Tuning.cpp
    Source/tuning/Strummer.cpp)
target_include_directories(zither_core PUBLIC Source)
set_target_properties(zither_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(zither_core PRIVATE /W4)
else()
    target_compile_options(zither_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

juce_add_plugin(Zither
    COMPANY_NAME                "Flow Instruments"
    BUNDLE_ID                   com.flowinstruments.zither
    PLUGIN_MANUFACTURER_CODE    Flwi
    PLUGIN_CODE                 Zthr
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "Zither"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     ${ZITHER_COPY_PLUGIN_AFTER_BUILD}
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

# Shared with the plugin state tests, which build them into a console app.
set(ZITHER_PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/fx/FxChain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/AquaLookAndFeel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/ControlPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/StringField.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/WaterScene.cpp)
set(ZITHER_JUCE_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
set(ZITHER_JUCE_MODULES
    juce::juce_audio_utils
    juce::juce_dsp)

target_sources(Zither PRIVATE ${ZITHER_PLUGIN_SOURCES})

target_compile_definitions(Zither PUBLIC
    ${ZITHER_JUCE_DEFINITIONS}
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Zither
    PRIVATE
        zither_core
        ${ZITHER_JUCE_MODULES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(ZITHER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
