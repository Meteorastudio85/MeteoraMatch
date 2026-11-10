#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// =============================================================================
//  La grafica di sfondo (Assets/background.png, 868x584) contiene tutte le parti
//  fisse del mockup. Sopra vengono disegnati, nelle stesse posizioni, gli
//  elementi che cambiano: curva, manopole, LED, meter e valori.
// =============================================================================

namespace UiColours
{
    inline const juce::Colour accent    { 0xffff9a2e };
    inline const juce::Colour accentHot { 0xffffc27a };
    inline const juce::Colour matchText { 0xffffb066 };
    inline const juce::Colour teal      { 0xff2fb8c4 };
    inline const juce::Colour text      { 0xffe6e8eb };
    inline const juce::Colour textDim   { 0xff9aa0a8 };
    inline const juce::Colour track     { 0xff2a2d33 };
    inline const juce::Colour ledOff    { 0xff3b3e44 };
}

// =============================================================================
class MeteoraLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MeteoraLookAndFeel();

    juce::Font font (float height, bool semibold = false) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

private:
    void drawBigKnob   (juce::Graphics&, juce::Rectangle<float>, float pos, float a0, float a1, const juce::String& value);
    void drawSmallKnob (juce::Graphics&, juce::Rectangle<float>, float pos, float a0, float a1);
    void drawDotsKnob  (juce::Graphics&, juce::Rectangle<float>, float pos, float a0, float a1, bool bipolar);

    juce::Typeface::Ptr regular, semibold;
};

// =============================================================================
//  Display: curva del target (bianca), sorgente corretta (teal)
// =============================================================================
class CurveDisplay final : public juce::Component
{
public:
    CurveDisplay (MicMatchAudioProcessor&, MeteoraLookAndFeel&);

    void refreshIfChanged();
    void paint (juce::Graphics&) override;

private:
    MicMatchAudioProcessor& processor;
    MeteoraLookAndFeel& lnf;
    MatchSettings lastSettings;
    MicMatchEngine::Stages stages;
};

// =============================================================================
//  LED cliccabili collegati ai parametri
// =============================================================================
class ModeSelector final : public juce::Component
{
public:
    explicit ModeSelector (juce::RangedAudioParameter& modeParam);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    int index = 0;
};

class LedToggle final : public juce::Component
{
public:
    LedToggle (juce::RangedAudioParameter& param, const juce::String& text, MeteoraLookAndFeel&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    juce::String label;
    MeteoraLookAndFeel& lnf;
    bool on = false;
};

// =============================================================================
//  Pannello meter in basso. Clic per azzerare i valori massimi.
// =============================================================================
class MeterPanel final : public juce::Component
{
public:
    explicit MeterPanel (MeteoraLookAndFeel& l) : lnf (l) {}

    void setLevels (float inPeak, float inRms, float outPeak, float outRms);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    MeteoraLookAndFeel& lnf;
    float inPk = -60.0f, inRms = -60.0f, outPk = -60.0f, outRms = -60.0f;
    float outPkMax = -60.0f, outRmsMax = -60.0f;
};

// =============================================================================
class MicMatchAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit MicMatchAudioProcessorEditor (MicMatchAudioProcessor&);
    ~MicMatchAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, const char* style, const juce::String& suffix, bool bipolar = false);

    MicMatchAudioProcessor& proc;
    MeteoraLookAndFeel lnf;
    juce::Image background;

    CurveDisplay curve;
    juce::Slider matchKnob, trimKnob, outputKnob, detailKnob, smoothKnob;
    ModeSelector modeSelector;
    LedToggle rolloffToggle, presenceToggle;
    MeterPanel meters;

    float inPeakDb = -60.0f, inRmsDb = -60.0f, outPeakDb = -60.0f, outRmsDb = -60.0f;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> matchAtt, trimAtt, outputAtt, detailAtt, smoothAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicMatchAudioProcessorEditor)
};
