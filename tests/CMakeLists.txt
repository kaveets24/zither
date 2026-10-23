FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(zither_tests
    WaveguideStringTests.cpp
    TuningTests.cpp)
target_link_libraries(zither_tests PRIVATE zither_core Catch2::Catch2WithMain)

# Session save/load compatibility. This links JUCE and the whole plugin, so it is a separate, slower target
# that keeps zither_tests quick to iterate on.
juce_add_console_app(zither_plugin_tests)
target_sources(zither_plugin_tests PRIVATE PluginStateTests.cpp ${ZITHER_PLUGIN_SOURCES})
target_compile_definitions(zither_plugin_tests PRIVATE
    ${ZITHER_JUCE_DEFINITIONS}
    JucePlugin_Name="Zither")
target_link_libraries(zither_plugin_tests
    PRIVATE
        zither_core
        ${ZITHER_JUCE_MODULES}
        Catch2::Catch2WithMain
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(zither_tests)
catch_discover_tests(zither_plugin_tests)
