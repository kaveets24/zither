cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Zither VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
if(NOT MSVC)
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
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice)

target_sources(Zither PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/fx/FxChain.cpp
    Source/ui/AquaLookAndFeel.cpp
    Source/ui/ControlPanel.cpp
    Source/ui/StringField.cpp
    Source/ui/WaterScene.cpp)

target_compile_definitions(Zither PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Zither
    PRIVATE
        zither_core
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(ZITHER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
