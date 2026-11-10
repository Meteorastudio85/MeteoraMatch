cmake_minimum_required(VERSION 3.22)

project(MeteoraMicMatch VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: scaricato automaticamente al primo configure.
# Se hai gia' JUCE in locale, sostituisci con: add_subdirectory(/percorso/JUCE JUCE)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(MICMATCH_FORMATS AU VST3 Standalone)
else()
    set(MICMATCH_FORMATS VST3 Standalone)
endif()

juce_add_plugin(MeteoraMicMatch
    COMPANY_NAME                "Meteora Plugins"
    BUNDLE_ID                   com.meteoraplugins.micmatch
    PLUGIN_MANUFACTURER_CODE    Mtra
    PLUGIN_CODE                 Mmbd
    FORMATS                     ${MICMATCH_FORMATS}
    PRODUCT_NAME                "Meteora MicMatch"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx EQ
    AU_MAIN_TYPE                kAudioUnitType_Effect)

juce_add_binary_data(MicMatchAssets
    SOURCES
        Assets/background.png
        Assets/Inter-Regular.otf
        Assets/Inter-SemiBold.otf)

target_sources(MeteoraMicMatch PRIVATE
    Source/MicMatchEngine.cpp
    Source/Parameters.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MeteoraMicMatch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MeteoraMicMatch
    PRIVATE
        MicMatchAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
