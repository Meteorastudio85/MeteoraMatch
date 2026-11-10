#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MicMatchEngine.h"
#include "Parameters.h"

class MicMatchAudioProcessor final : public juce::AudioProcessor
{
public:
    MicMatchAudioProcessor();
    ~MicMatchAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // usato da audio thread e GUI (legge solo atomici)
    MatchSettings readSettings() const;
    double getProcessingSampleRate() const { return processingSampleRate.load(); }

    struct Meter
    {
        std::atomic<float> peak { 0.0f };   // massimo dall'ultima lettura della GUI
        std::atomic<float> rms  { 0.0f };   // ultimo blocco
    };

    Meter inputMeter, outputMeter;
    juce::AudioProcessorValueTreeState apvts;

private:
    static void applySmoothedGain (juce::AudioBuffer<float>&, int numChannels,
                                   juce::SmoothedValue<float>& gain);
    static void measure (const juce::AudioBuffer<float>&, int numChannels, Meter&);

    MicMatchEngine engine;
    juce::SmoothedValue<float> inGain, outGain;
    std::atomic<double> processingSampleRate { 48000.0 };

    std::atomic<float>* pInputTrim   = nullptr;
    std::atomic<float>* pOutputLevel = nullptr;
    std::atomic<float>* pMatch       = nullptr;
    std::atomic<float>* pDetail      = nullptr;
    std::atomic<float>* pSmooth      = nullptr;
    std::atomic<float>* pMode        = nullptr;
    std::atomic<float>* pBassRolloff = nullptr;
    std::atomic<float>* pPresence    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicMatchAudioProcessor)
};
