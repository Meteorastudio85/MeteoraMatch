#include "Parameters.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    const auto dbAttr  = AudioParameterFloatAttributes().withLabel ("dB");
    const auto pctAttr = AudioParameterFloatAttributes().withLabel ("%")
                             .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)); });

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::inputTrim, 1 }, "Input Trim",
        NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f, dbAttr));

    NormalisableRange<float> outRange (outputMinDb, 12.0f, 0.1f);
    outRange.setSkewForCentre (0.0f);

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::outputLevel, 1 }, "Output Level", outRange, 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int)
            {
                return v <= outputMinDb ? String ("-inf") : String (v, 1);
            })));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::match, 1 }, "Match",
        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, pctAttr));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::detail, 1 }, "Detail",
        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, pctAttr));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::smooth, 1 }, "Smooth",
        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, pctAttr));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::mode, 1 }, "Mode", StringArray { "Vocals", "Speech" }, 0));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::bassRolloff, 1 }, "Bass Roll-off", false));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::presence, 1 }, "Presence", false));

    return layout;
}
