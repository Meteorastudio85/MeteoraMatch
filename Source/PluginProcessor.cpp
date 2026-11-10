#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    float outputDbToGain (float db)
    {
        return db <= outputMinDb ? 0.0f : juce::Decibels::decibelsToGain (db);
    }
}

MicMatchAudioProcessor::MicMatchAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    pInputTrim   = apvts.getRawParameterValue (ParamIDs::inputTrim);
    pOutputLevel = apvts.getRawParameterValue (ParamIDs::outputLevel);
    pMatch       = apvts.getRawParameterValue (ParamIDs::match);
    pDetail      = apvts.getRawParameterValue (ParamIDs::detail);
    pSmooth      = apvts.getRawParameterValue (ParamIDs::smooth);
    pMode        = apvts.getRawParameterValue (ParamIDs::mode);
    pBassRolloff = apvts.getRawParameterValue (ParamIDs::bassRolloff);
    pPresence    = apvts.getRawParameterValue (ParamIDs::presence);
}

MatchSettings MicMatchAudioProcessor::readSettings() const
{
    MatchSettings s;
    s.match        = pMatch->load()  / 100.0f;
    s.detail       = pDetail->load() / 100.0f;
    s.smooth       = pSmooth->load() / 100.0f;
    s.lowExtension = juce::roundToInt (pMode->load()) == 1;   // Speech
    s.bassRolloff  = pBassRolloff->load() > 0.5f;
    s.presence     = pPresence->load() > 0.5f;
    return s;
}

void MicMatchAudioProcessor::prepareToPlay (double sampleRate, int)
{
    processingSampleRate = sampleRate;

    engine.prepare (sampleRate, getTotalNumOutputChannels());
    engine.setTarget (readSettings(), true);
    engine.reset();

    inGain.reset (sampleRate, 0.05);
    outGain.reset (sampleRate, 0.05);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pInputTrim->load()));
    outGain.setCurrentAndTargetValue (outputDbToGain (pOutputLevel->load()));
}

bool MicMatchAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void MicMatchAudioProcessor::applySmoothedGain (juce::AudioBuffer<float>& buffer, int numChannels,
                                                juce::SmoothedValue<float>& gain)
{
    const int n = buffer.getNumSamples();

    if (! gain.isSmoothing())
    {
        const float g = gain.getTargetValue();
        if (g != 1.0f)
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.applyGain (ch, 0, n, g);
        return;
    }

    auto* const* data = buffer.getArrayOfWritePointers();

    for (int i = 0; i < n; ++i)
    {
        const float g = gain.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            data[ch][i] *= g;
    }
}

void MicMatchAudioProcessor::measure (const juce::AudioBuffer<float>& buffer, int numChannels, Meter& meter)
{
    const int n = buffer.getNumSamples();
    if (n == 0 || numChannels == 0)
        return;

    float peak = 0.0f;
    double sumSq = 0.0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* x = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            peak = std::max (peak, std::abs (x[i]));
            sumSq += (double) x[i] * x[i];
        }
    }

    float prev = meter.peak.load();
    while (peak > prev && ! meter.peak.compare_exchange_weak (prev, peak)) {}

    meter.rms.store ((float) std::sqrt (sumSq / (double) (n * numChannels)));
}

void MicMatchAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    const int numCh = std::min (numIn, numOut);

    inGain.setTargetValue (juce::Decibels::decibelsToGain (pInputTrim->load()));
    outGain.setTargetValue (outputDbToGain (pOutputLevel->load()));

    applySmoothedGain (buffer, numCh, inGain);
    measure (buffer, numCh, inputMeter);

    engine.setTarget (readSettings());
    engine.process (buffer.getArrayOfWritePointers(), numCh, n);

    applySmoothedGain (buffer, numCh, outGain);
    measure (buffer, numCh, outputMeter);
}

void MicMatchAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MicMatchAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MicMatchAudioProcessor::createEditor()
{
    return new MicMatchAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicMatchAudioProcessor();
}
