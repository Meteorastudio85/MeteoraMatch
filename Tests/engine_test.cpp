// Test: risposta teorica vs impulso processato, a 44.1 e 48 kHz
#include "../Source/MicMatchEngine.h"
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

static double measuredDb (const std::vector<float>& h, double f, double fs)
{
    std::complex<double> acc = 0;
    for (size_t n = 0; n < h.size(); ++n)
        acc += (double) h[n] * std::polar (1.0, -2.0 * M_PI * f * (double) n / fs);
    return 20.0 * std::log10 (std::abs (acc));
}

int main()
{
    const double freqs[] = { 60, 100, 150, 300, 1000, 2500, 3000, 4000, 5000, 8000, 10000, 12000, 14000 };

    for (double fs : { 44100.0, 48000.0 })
    {
        MatchSettings s;
        MicMatchEngine e;
        e.prepare (fs, 2);
        e.setTarget (s, true);

        std::vector<float> L (16384, 0.0f), R (16384, 0.0f);
        L[0] = R[0] = 1.0f;
        float* ch[] = { L.data(), R.data() };
        e.process (ch, 2, (int) L.size());

        const auto st = MicMatchEngine::computeStages (s);
        double maxErr = 0;
        std::printf ("fs %.0f\n  Hz      teorico  misurato\n", fs);
        for (double f : freqs)
        {
            const double t = MicMatchEngine::magnitudeDb (st, f, fs);
            const double m = measuredDb (L, f, fs);
            maxErr = std::max (maxErr, std::abs (t - m));
            std::printf ("  %-7.0f %6.2f   %6.2f\n", f, t, m);
        }
        std::printf ("  max diff teorico/misurato: %.4f dB, L==R: %s\n\n",
                     maxErr, L == R ? "si" : "no");
    }

    // Match 0 deve essere trasparente
    MatchSettings zero; zero.match = 0.0f;
    const auto st0 = MicMatchEngine::computeStages (zero);
    double worst = 0;
    for (double f = 20; f < 20000; f *= 1.1)
        worst = std::max (worst, std::abs (MicMatchEngine::magnitudeDb (st0, f, 48000)));
    std::printf ("Match 0%%: deviazione max %.6f dB\n", worst);

    // Speech + switch
    MatchSettings sp; sp.lowExtension = true; sp.bassRolloff = true; sp.presence = true;
    const auto st1 = MicMatchEngine::computeStages (sp);
    std::printf ("Speech+rolloff+presence @50/100/200/5k Hz: %.1f %.1f %.1f %.1f dB\n",
                 MicMatchEngine::magnitudeDb (st1, 50, 48000), MicMatchEngine::magnitudeDb (st1, 100, 48000),
                 MicMatchEngine::magnitudeDb (st1, 200, 48000), MicMatchEngine::magnitudeDb (st1, 5000, 48000));

    // Automazione veloce: nessun NaN / esplosione
    MicMatchEngine e; e.prepare (48000, 1);
    std::vector<float> noise (48000);
    unsigned r = 1;
    for (auto& v : noise) { r = r * 1664525u + 1013904223u; v = (float) ((r >> 9) / 4194304.0 - 1.0) * 0.5f; }
    float peak = 0;
    for (int b = 0; b < 48000 / 256; ++b)
    {
        MatchSettings m; m.match = (b % 2) ? 1.0f : 0.0f; m.smooth = (b % 3) / 2.0f; m.presence = (b % 5) == 0;
        e.setTarget (m);
        float* p[] = { noise.data() + b * 256 };
        e.process (p, 1, 256);
        for (int i = 0; i < 256; ++i) { if (! std::isfinite (p[0][i])) { std::puts ("NaN!"); return 1; } peak = std::max (peak, std::abs (p[0][i])); }
    }
    std::printf ("Stress automazione: picco %.2f, nessun NaN\n", peak);
}
