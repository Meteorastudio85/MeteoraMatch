#pragma once

// =============================================================================
//  Risposte in frequenza digitalizzate dai datasheet (switch in posizione flat),
//  normalizzate a 0 dB a 1 kHz. Usate solo dalla GUI per disegnare le curve.
//  Valide fra 50 Hz e 18 kHz.
// =============================================================================

#include <cmath>
#include <cstddef>

namespace MicProfiles
{
    struct Point { double hz, db; };

    inline constexpr double minHz = 50.0;
    inline constexpr double maxHz = 18000.0;

    // sorgente: dinamico tipo BC500 (misura a 1/12 di ottava)
    inline constexpr Point source[] = {
        { 50, -17 }, { 55, -13 }, { 60, -10 }, { 65, -6.5 }, { 70, -4.8 }, { 80, -3 }, { 90, -1.6 },
        { 100, -1 }, { 120, -1 }, { 140, 0 }, { 160, -0.8 }, { 200, -0.8 }, { 250, -0.5 }, { 300, -0.9 },
        { 400, 0 }, { 500, 0 }, { 600, 0.5 }, { 700, 0 }, { 1000, 0 }, { 1500, 0 }, { 2000, 0.2 },
        { 2500, 1.5 }, { 3000, 2.5 }, { 3500, 4 }, { 4000, 3 }, { 5000, 3 }, { 6000, 3.8 }, { 6500, 0 },
        { 6800, -5 }, { 7200, 2 }, { 7500, 4.7 }, { 8000, 4 }, { 9000, 2 }, { 10000, 0 }, { 12000, -4 },
        { 14000, -10 }, { 16000, -5 }, { 18000, -6 }
    };

    // target: dinamico classico da broadcast tipo SM7B
    inline constexpr Point target[] = {
        { 50, -5 }, { 60, -4.2 }, { 70, -3.6 }, { 80, -3.2 }, { 100, -2.5 }, { 150, -1.8 }, { 200, -1.5 },
        { 300, -1.2 }, { 500, -1 }, { 700, -0.5 }, { 1000, 0 }, { 2000, 0 }, { 3000, 0.5 }, { 4000, 1 },
        { 5000, 1 }, { 6000, 1.8 }, { 7000, 0.5 }, { 8000, 1 }, { 10000, 0 }, { 12000, -1 },
        { 14000, -5 }, { 15000, -9 }, { 17000, -14 }, { 18000, -14 }
    };

    // interpolazione lineare in dB su asse logaritmico delle frequenze
    template <std::size_t N>
    double interpolate (const Point (&p)[N], double hz)
    {
        if (hz <= p[0].hz)     return p[0].db;
        if (hz >= p[N - 1].hz) return p[N - 1].db;

        for (std::size_t i = 1; i < N; ++i)
        {
            if (hz <= p[i].hz)
            {
                const double t = std::log (hz / p[i - 1].hz) / std::log (p[i].hz / p[i - 1].hz);
                return p[i - 1].db + t * (p[i].db - p[i - 1].db);
            }
        }

        return p[N - 1].db;
    }
}
