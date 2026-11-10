#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr const char* inputTrim   = "inputTrim";
    inline constexpr const char* outputLevel = "outputLevel";
    inline constexpr const char* match       = "match";
    inline constexpr const char* detail      = "detail";
    inline constexpr const char* smooth      = "smooth";
    inline constexpr const char* mode        = "mode";         // 0 = Vocals, 1 = Speech
    inline constexpr const char* bassRolloff = "bassRolloff";
    inline constexpr const char* presence    = "presence";
}

// L'uscita al minimo vale -inf (mute)
inline constexpr float outputMinDb = -60.0f;

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
