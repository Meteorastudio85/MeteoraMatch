#include "MicMatchEngine.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace
{
    constexpr double pi = 3.14159265358979323846;

    double clamp01 (double v) { return std::min (1.0, std::max (0.0, v)); }
}

bool operator== (const MatchSettings& a, const MatchSettings& b)
{
    return a.match == b.match && a.detail == b.detail && a.smooth == b.smooth
        && a.lowExtension == b.lowExtension && a.bassRolloff == b.bassRolloff
        && a.presence == b.presence;
}

// =============================================================================
//  Profilo
// =============================================================================
MicMatchEngine::Stages MicMatchEngine::computeStages (const MatchSettings& s)
{
    const double m  = clamp01 (s.match);
    const double d  = clamp01 (s.detail);
    const double qs = 1.0 - 0.6 * clamp01 (s.smooth);   // Smooth al 100% -> Q x 0.4

    auto q = [qs] (double baseQ) { return std::max (0.2, baseQ * qs); };

    using T = StageType;

    return {{
        // --- correzione principale (scalata da Match) -------------------------
        { T::Peak,        130.0, -1.2 * m,     q (0.30), true },
        { T::Peak,       3915.0, -3.3 * m,     q (1.27), true },
        { T::Peak,       8660.0, -4.5 * m * d, q (2.55), true },
        { T::HighShelf,  9000.0, +5.3 * m,     0.5,      true },

        // --- Mode "Speech": estensione basse + HPF di protezione --------------
        { T::LowShelf,     60.0, +4.0 * m,     0.7,      s.lowExtension },
        { T::HighPass,     30.0,  0.0,         0.707,    s.lowExtension },

        // --- emulazione degli switch del target (approssimate dal datasheet) --
        { T::HighPass,    140.0,  0.0,         0.55,     s.bassRolloff },
        { T::Peak,       4500.0, +3.5,         0.9,      s.presence },
    }};
}

// =============================================================================
//  Biquad RBJ (Audio EQ Cookbook) - stesse formule usate nel fit
// =============================================================================
MicMatchEngine::Biquad MicMatchEngine::design (const Stage& st, double sampleRate)
{
    const double f     = std::min (st.freq, sampleRate * 0.45);
    const double w     = 2.0 * pi * f / sampleRate;
    const double c     = std::cos (w);
    const double alpha = std::sin (w) / (2.0 * st.q);
    const double A     = std::pow (10.0, st.gainDb / 40.0);

    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (st.type)
    {
        case StageType::Peak:
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * c;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * c;
            a2 = 1.0 - alpha / A;
            break;

        case StageType::LowShelf:
        {
            const double sa = 2.0 * std::sqrt (A) * alpha;
            b0 =        A * ((A + 1) - (A - 1) * c + sa);
            b1 =  2.0 * A * ((A - 1) - (A + 1) * c);
            b2 =        A * ((A + 1) - (A - 1) * c - sa);
            a0 =             (A + 1) + (A - 1) * c + sa;
            a1 = -2.0 *     ((A - 1) + (A + 1) * c);
            a2 =             (A + 1) + (A - 1) * c - sa;
            break;
        }

        case StageType::HighShelf:
        {
            const double sa = 2.0 * std::sqrt (A) * alpha;
            b0 =        A * ((A + 1) + (A - 1) * c + sa);
            b1 = -2.0 * A * ((A - 1) + (A + 1) * c);
            b2 =        A * ((A + 1) + (A - 1) * c - sa);
            a0 =             (A + 1) - (A - 1) * c + sa;
            a1 =  2.0 *     ((A - 1) - (A + 1) * c);
            a2 =             (A + 1) - (A - 1) * c - sa;
            break;
        }

        case StageType::HighPass:
            b0 =  (1.0 + c) / 2.0;
            b1 = -(1.0 + c);
            b2 =  (1.0 + c) / 2.0;
            a0 =  1.0 + alpha;
            a1 = -2.0 * c;
            a2 =  1.0 - alpha;
            break;
    }

    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

double MicMatchEngine::magnitudeDb (const Stages& stages, double freqHz, double sampleRate)
{
    const double w = 2.0 * pi * freqHz / sampleRate;
    const std::complex<double> z1 = std::polar (1.0, -w);
    const std::complex<double> z2 = z1 * z1;

    double totalDb = 0.0;

    for (const auto& st : stages)
    {
        if (! st.enabled)
            continue;

        const auto b = design (st, sampleRate);
        const auto num = b.b0 + b.b1 * z1 + b.b2 * z2;
        const auto den = 1.0  + b.a1 * z1 + b.a2 * z2;
        totalDb += 20.0 * std::log10 (std::max (1.0e-12, std::abs (num / den)));
    }

    return totalDb;
}

// =============================================================================
//  Smoother
// =============================================================================
void MicMatchEngine::LinearSmoother::setRampLength (int samples)
{
    rampLength = std::max (1, samples);
    setCurrentAndTarget (target);
}

void MicMatchEngine::LinearSmoother::setCurrentAndTarget (float v)
{
    current = target = v;
    countdown = 0;
}

void MicMatchEngine::LinearSmoother::setTarget (float v)
{
    if (v == target)
        return;

    target = v;
    countdown = rampLength;
    step = (target - current) / (float) rampLength;
}

float MicMatchEngine::LinearSmoother::skip (int numSamples)
{
    if (numSamples >= countdown)
    {
        countdown = 0;
        current = target;
        return current;
    }

    countdown -= numSamples;
    current += step * (float) numSamples;
    return current;
}

// =============================================================================
//  Elaborazione
// =============================================================================
void MicMatchEngine::prepare (double sampleRate, int maxChannels)
{
    fs = sampleRate;
    state.assign ((size_t) std::max (1, maxChannels), {});

    const int ramp = (int) (0.05 * fs);   // 50 ms
    match.setRampLength (ramp);
    detail.setRampLength (ramp);
    smooth.setRampLength (ramp);

    active.fill (false);
    dirty = true;
}

void MicMatchEngine::reset()
{
    for (auto& ch : state)
        ch.fill ({});
}

void MicMatchEngine::setTarget (const MatchSettings& s, bool snap)
{
    if (snap)
    {
        match.setCurrentAndTarget (s.match);
        detail.setCurrentAndTarget (s.detail);
        smooth.setCurrentAndTarget (s.smooth);
        dirty = true;
    }
    else
    {
        match.setTarget (s.match);
        detail.setTarget (s.detail);
        smooth.setTarget (s.smooth);
    }

    if (s.lowExtension != target.lowExtension
        || s.bassRolloff != target.bassRolloff
        || s.presence != target.presence)
        dirty = true;

    target = s;
}

void MicMatchEngine::updateCoefficients (const MatchSettings& s)
{
    const auto stages = computeStages (s);

    for (int k = 0; k < numStages; ++k)
    {
        const bool enable = stages[(size_t) k].enabled;

        if (enable && ! active[(size_t) k])          // stadio appena attivato: stato pulito
            for (auto& ch : state)
                ch[(size_t) k] = {};

        active[(size_t) k] = enable;

        if (enable)
            coeffs[(size_t) k] = design (stages[(size_t) k], fs);
    }
}

void MicMatchEngine::process (float* const* channels, int numChannels, int numSamples)
{
    numChannels = std::min (numChannels, (int) state.size());

    for (int pos = 0; pos < numSamples; pos += subBlock)
    {
        const int len = std::min (subBlock, numSamples - pos);

        if (dirty || match.isSmoothing() || detail.isSmoothing() || smooth.isSmoothing())
        {
            MatchSettings s = target;
            s.match  = match.skip (len);
            s.detail = detail.skip (len);
            s.smooth = smooth.skip (len);
            updateCoefficients (s);
            dirty = false;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* x = channels[ch] + pos;
            auto& chState = state[(size_t) ch];

            for (int k = 0; k < numStages; ++k)
            {
                if (! active[(size_t) k])
                    continue;

                const auto& b = coeffs[(size_t) k];
                auto& z = chState[(size_t) k];

                // Transposed Direct Form II in double precision
                for (int i = 0; i < len; ++i)
                {
                    const double in  = x[i];
                    const double out = b.b0 * in + z.z1;
                    z.z1 = b.b1 * in - b.a1 * out + z.z2;
                    z.z2 = b.b2 * in - b.a2 * out;
                    x[i] = (float) out;
                }
            }
        }
    }
}
