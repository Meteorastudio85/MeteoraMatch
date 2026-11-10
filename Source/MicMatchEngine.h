#pragma once

// =============================================================================
//  MicMatchEngine
//  Motore di correzione BC-type -> classic broadcast dynamic.
//  C++ puro (nessuna dipendenza da JUCE) per poterlo testare da solo.
//
//  Profilo ricavato dalle curve dei datasheet (switch in posizione flat),
//  differenza smussata a 1/3 di ottava, fit 90 Hz - 13 kHz, errore RMS ~0.4 dB.
// =============================================================================

#include <array>
#include <vector>

struct MatchSettings
{
    float match  = 1.0f;          // 0..1  quantita' di correzione
    float detail = 1.0f;          // 0..1  quanto della banda stretta a 8.7 kHz applicare
    float smooth = 0.0f;          // 0..1  allarga le campane (correzione piu' morbida)
    bool  lowExtension = false;   // Mode "Speech": recupera il sub-basso sotto i 70 Hz
    bool  bassRolloff  = false;   // emulazione switch roll-off del target
    bool  presence     = false;   // emulazione switch presence boost del target
};

bool operator== (const MatchSettings& a, const MatchSettings& b);
inline bool operator!= (const MatchSettings& a, const MatchSettings& b) { return ! (a == b); }

class MicMatchEngine
{
public:
    enum class StageType { Peak, LowShelf, HighShelf, HighPass };

    struct Stage
    {
        StageType type;
        double freq;
        double gainDb;
        double q;
        bool enabled;
    };

    struct Biquad   // coefficienti normalizzati (a0 = 1)
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };

    static constexpr int numStages = 8;
    using Stages = std::array<Stage, numStages>;

    // --- funzioni statiche, usate anche dalla GUI per disegnare la curva -----
    static Stages computeStages (const MatchSettings& settings);
    static Biquad design (const Stage& stage, double sampleRate);
    static double magnitudeDb (const Stages& stages, double freqHz, double sampleRate);

    // --- elaborazione ---------------------------------------------------------
    void prepare (double sampleRate, int maxChannels);
    void reset();

    // snap = true salta lo smoothing (da usare in prepare)
    void setTarget (const MatchSettings& settings, bool snap = false);

    void process (float* const* channels, int numChannels, int numSamples);

private:
    struct LinearSmoother
    {
        void setRampLength (int samples);
        void setCurrentAndTarget (float v);
        void setTarget (float v);
        bool isSmoothing() const { return countdown > 0; }
        float skip (int numSamples);

        float current = 0.0f, target = 0.0f, step = 0.0f;
        int countdown = 0, rampLength = 1;
    };

    struct FilterState { double z1 = 0.0, z2 = 0.0; };

    void updateCoefficients (const MatchSettings& settings);

    std::array<Biquad, numStages> coeffs {};
    std::array<bool, numStages> active {};
    std::vector<std::array<FilterState, numStages>> state;

    LinearSmoother match, detail, smooth;
    MatchSettings target;
    bool dirty = true;
    double fs = 48000.0;

    static constexpr int subBlock = 32;   // aggiornamento coefficienti ogni 32 campioni
};
