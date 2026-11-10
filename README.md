# Meteora MicMatch

Plugin che fa suonare un dinamico da broadcast tipo BC500 come un classico dinamico da broadcast tipo SM7B.
Formati: VST3 e Standalone; su macOS anche AU.

## Compilare

Serve CMake 3.22 o superiore e un compilatore C++17 (Xcode, Visual Studio 2022 o gcc/clang).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Al primo `cmake` viene scaricato JUCE 8.0.4. Se hai già JUCE in locale, nel `CMakeLists.txt` sostituisci il blocco `FetchContent` con `add_subdirectory(/percorso/JUCE JUCE)`.

I plugin compilati finiscono in `build/MeteoraMicMatch_artefacts/Release/`.

## Struttura

| File | Cosa fa |
|---|---|
| `Source/MicMatchEngine.*` | Motore DSP in C++ puro: profilo di correzione, biquad RBJ, smoothing |
| `Source/Parameters.*` | Parametri e range |
| `Source/PluginProcessor.*` | Collegamento a JUCE: trim, motore, output, meter, salvataggio stato |
| `Source/PluginEditor.*` | GUI: sfondo del mockup + curva, manopole, LED e meter disegnati sopra |
| `Source/MicProfiles.h` | Curve dei due microfoni dai datasheet, usate per il display |
| `Assets/background.png` | Grafica fissa della GUI (868×584), ricavata dal mockup |
| `Assets/Inter-*.otf` | Font Inter (licenza OFL, vedi `Inter-LICENSE.txt`) |
| `Assets/meteora_logo_light.png` | Logo Meteora chiaro su trasparente, già inserito nello sfondo |
| `Assets/mic_source.svg` | Icona vettoriale del microfono sorgente |
| `Tests/engine_test.cpp` | Test del motore, senza JUCE |

Il test del motore si compila e lancia così:

```bash
g++ -std=c++17 -O2 Tests/engine_test.cpp Source/MicMatchEngine.cpp -o engine_test && ./engine_test
```

## Catena del segnale

Input Trim → meter In → correzione → Output Level → meter Out

## Profilo di correzione

Il profilo è ricavato dai datasheet, con gli switch in posizione flat. La differenza è smussata a 1/3 di ottava. Il fit copre 90 Hz – 13 kHz con un errore medio di circa 0,4 dB.

| Stadio | Tipo | Freq | Gain | Q | Controllato da |
|---|---|---|---|---|---|
| 1 | Peak | 130 Hz | −1,2 dB | 0,30 | Match, Smooth |
| 2 | Peak | 3,9 kHz | −3,3 dB | 1,27 | Match, Smooth |
| 3 | Peak | 8,7 kHz | −4,5 dB | 2,55 | Match, Detail, Smooth |
| 4 | High shelf | 9 kHz | +5,3 dB | 0,5 | Match |
| 5 | Low shelf | 60 Hz | +4 dB | 0,7 | Mode Speech, Match |
| 6 | HPF | 30 Hz | – | 0,707 | Mode Speech |
| 7 | HPF | 140 Hz | – | 0,55 | Bass roll-off |
| 8 | Peak | 4,5 kHz | +3,5 dB | 0,9 | Presence |

Gli stadi 7 e 8 approssimano gli switch del target dalle curve tratteggiate del datasheet. Vanno verificati a orecchio.

## Parametri

| Parametro | Range | Effetto |
|---|---|---|
| Match | 0–100 % | Quanta correzione applicare. A 0 % il plugin è trasparente |
| Detail | 0–100 % | Quanto applicare la banda stretta a 8,7 kHz |
| Smooth | 0–100 % | Allarga le campane, fino a Q × 0,4, per una correzione più morbida |
| Mode | Vocals / Speech | Speech recupera il sub-basso sotto i 70 Hz che il BC500 taglia, e aggiunge un HPF a 30 Hz di protezione |
| Bass roll-off | on/off | Emula lo switch roll-off del target |
| Presence | on/off | Emula lo switch presence del target |
| Input Trim | ±6 dB | Guadagno prima della correzione |
| Output Level | −∞ … +12 dB | Guadagno finale |

Match, Detail e Smooth cambiano in modo graduale (rampa di 50 ms, coefficienti aggiornati ogni 32 campioni), quindi si possono automatizzare senza click. La latenza è zero.

## Da fare

- Esportare lo sfondo anche a 2× (1736×1168) per gli schermi ad alta densità.
- Provare il profilo a orecchio su voci reali e ritoccare i valori in `MicMatchEngine::computeStages()`. Tutti i numeri del profilo sono lì.
- Se possibile, misurare i due microfoni e rifare il fit con dati reali.

## Build automatica su GitHub

Il file `.github/workflows/build.yml` compila il plugin per Windows x64 (VST3 e Standalone) su GitHub Actions.

- **A ogni push su `main`** parte prima il test del motore DSP, poi la build. Lo zip si scarica dalla pagina della run, nella scheda *Actions* del repository, sezione *Artifacts*.
- **Con un tag** tipo `v0.1.0` viene creata anche una bozza di Release con lo zip allegato:

```bash
git tag v0.1.0
git push origin v0.1.0
```

La build non è firmata. Per la vendita la firma del codice su Windows è consigliata ma non obbligatoria.
