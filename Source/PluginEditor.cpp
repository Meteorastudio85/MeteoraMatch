#include "PluginEditor.h"
#include "MicProfiles.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
    constexpr int editorWidth  = 868;
    constexpr int editorHeight = 584;

    // posizioni prese dal mockup (coordinate della finestra)
    const Rectangle<int> curveBounds   { 190, 17, 489, 184 };
    const Rectangle<int> matchBounds   { 336, 285, 198, 198 };
    const Rectangle<int> trimBounds    { 226, 356, 76, 76 };
    const Rectangle<int> outputBounds  { 566, 357, 76, 76 };
    const Rectangle<int> detailBounds  { 450, 204, 44, 44 };
    const Rectangle<int> smoothBounds  { 597, 204, 44, 44 };
    const Rectangle<int> modeBounds    { 279, 207, 24, 46 };
    const Rectangle<int> meterBounds   { 0, 490, 868, 94 };

    String dbText (float db)
    {
        return db <= -59.9f ? String ("-inf") : String (db, 1);
    }

    void drawLed (Graphics& g, Point<float> centre, bool lit)
    {
        // quadratino di fondo come nel mockup, copre il LED disegnato nello sfondo
        g.setColour (Colour (0xff24272c));
        g.fillRoundedRectangle (Rectangle<float> (19.0f, 19.0f).withCentre (centre), 3.0f);

        const auto led = Rectangle<float> (11.0f, 11.0f).withCentre (centre);
        if (lit)
        {
            g.setColour (UiColours::accent.withAlpha (0.25f));
            g.fillEllipse (led.expanded (3.0f));
            g.setGradientFill (ColourGradient (UiColours::accentHot, centre.x, centre.y - 4.0f,
                                               UiColours::accent, centre.x, centre.y + 5.0f, false));
            g.fillEllipse (led);
        }
        else
        {
            g.setColour (UiColours::ledOff);
            g.fillEllipse (led);
            g.setColour (Colours::black.withAlpha (0.4f));
            g.drawEllipse (led, 1.0f);
        }
    }
}

// =============================================================================
//  Look & feel
// =============================================================================
MeteoraLookAndFeel::MeteoraLookAndFeel()
{
    regular  = Typeface::createSystemTypefaceFor (BinaryData::InterRegular_otf,  BinaryData::InterRegular_otfSize);
    semibold = Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_otf, BinaryData::InterSemiBold_otfSize);
    if (regular != nullptr)
        setDefaultSansSerifTypeface (regular);
}

Font MeteoraLookAndFeel::font (float height, bool bold) const
{
    const auto& face = bold ? semibold : regular;
    if (face == nullptr)
        return Font (FontOptions (height));

    return Font (FontOptions (face).withHeight (height));
}

void MeteoraLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                           float pos, float a0, float a1, Slider& slider)
{
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const auto style  = slider.getProperties()["knobStyle"].toString();

    if (style == "big")
        drawBigKnob (g, bounds, pos, a0, a1, slider.getTextFromValue (slider.getValue()));
    else if (style == "dots")
        drawDotsKnob (g, bounds, pos, a0, a1, (bool) slider.getProperties()["bipolar"]);
    else
        drawSmallKnob (g, bounds, pos, a0, a1);
}

void MeteoraLookAndFeel::drawBigKnob (Graphics& g, Rectangle<float> b, float pos, float a0, float a1,
                                      const String& value)
{
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    // ghiera
    g.setGradientFill (ColourGradient (Colour (0xff2c3036), c.x, c.y - R, Colour (0xff111316), c.x, c.y + R, false));
    g.fillEllipse (Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.05f));
    g.drawEllipse (Rectangle<float> (R * 2.0f - 1.0f, R * 2.0f - 1.0f).withCentre (c), 1.0f);

    // binario dell'arco
    const float arcR = R * 0.86f, w = R * 0.11f;
    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
    g.setColour (Colour (0xff141619));
    g.strokePath (track, PathStrokeType (w + 4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xff25282d));
    g.strokePath (track, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));

    // arco del valore con bagliore
    if (pos > 0.001f)
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, ang, true);

        g.setColour (UiColours::accent.withAlpha (0.10f));
        g.strokePath (arc, PathStrokeType (w * 2.6f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (UiColours::accent.withAlpha (0.20f));
        g.strokePath (arc, PathStrokeType (w * 1.7f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (ColourGradient (UiColours::accentHot, c.x, c.y - arcR,
                                           UiColours::accent, c.x, c.y + arcR, false));
        g.strokePath (arc, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));

        g.setColour (Colour (0xfffff1dc));
        g.fillEllipse (Rectangle<float> (w * 0.75f, w * 0.75f).withCentre (c.getPointOnCircumference (arcR, ang)));
    }

    // manopola interna
    const float kr = R * 0.66f;
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (Rectangle<float> (kr * 2.0f + 6.0f, kr * 2.0f + 6.0f).withCentre (c.translated (0.0f, 3.0f)));
    g.setGradientFill (ColourGradient (Colour (0xff3b3f46), c.x, c.y - kr, Colour (0xff1b1e22), c.x, c.y + kr, false));
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawEllipse (Rectangle<float> (kr * 2.0f - 2.0f, kr * 2.0f - 2.0f).withCentre (c), 1.0f);

    g.setColour (UiColours::accent);
    g.fillEllipse (Rectangle<float> (6.0f, 6.0f).withCentre (c.getPointOnCircumference (kr * 0.82f, ang)));

    // testo
    g.setColour (UiColours::matchText);
    g.setFont (font (R * 0.24f));
    g.drawText ("Match", Rectangle<float> (kr * 2.0f, R * 0.28f).withCentre (c.translated (0.0f, -R * 0.13f)),
                Justification::centred);
    g.drawText (value, Rectangle<float> (kr * 2.0f, R * 0.28f).withCentre (c.translated (0.0f, R * 0.17f)),
                Justification::centred);
}

void MeteoraLookAndFeel::drawSmallKnob (Graphics& g, Rectangle<float> b, float pos, float a0, float a1)
{
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const float arcR = R - 3.0f, w = 3.5f;

    g.setColour (Colour (0xff0d0f12));
    g.fillEllipse (Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (c));

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, a1, true);
    g.setColour (UiColours::track);
    g.strokePath (track, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));

    if (pos > 0.001f)
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, a0, ang, true);
        g.setColour (UiColours::accent.withAlpha (0.25f));
        g.strokePath (arc, PathStrokeType (w * 2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (UiColours::accent);
        g.strokePath (arc, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float kr = R * 0.64f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3e45), c.x, c.y - kr, Colour (0xff17191d), c.x, c.y + kr, false));
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));

    g.setColour (Colour (0xffe6e6e6));
    g.drawLine (Line<float> (c.getPointOnCircumference (kr * 0.25f, ang), c.getPointOnCircumference (kr * 0.85f, ang)), 2.0f);
}

void MeteoraLookAndFeel::drawDotsKnob (Graphics& g, Rectangle<float> b, float pos, float a0, float a1, bool bipolar)
{
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    g.setColour (Colour (0xff111316));
    g.fillEllipse (Rectangle<float> (R * 2.0f, R * 2.0f).withCentre (c));

    constexpr int numDots = 21;
    const float dotR = R - 5.0f;

    for (int i = 0; i < numDots; ++i)
    {
        const float t = (float) i / (float) (numDots - 1);
        const auto p = c.getPointOnCircumference (dotR, a0 + t * (a1 - a0));
        const bool lit = bipolar ? (t >= jmin (0.5f, pos) - 0.001f && t <= jmax (0.5f, pos) + 0.001f)
                                 : (t <= pos + 0.001f && pos > 0.001f);
        if (lit)
        {
            g.setColour (UiColours::accent.withAlpha (0.3f));
            g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (p));
            g.setColour (UiColours::accentHot);
        }
        else
        {
            g.setColour (UiColours::ledOff);
        }
        g.fillEllipse (Rectangle<float> (3.6f, 3.6f).withCentre (p));
    }

    const float kr = R * 0.64f;
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (Rectangle<float> (kr * 2.0f + 4.0f, kr * 2.0f + 4.0f).withCentre (c.translated (0.0f, 2.0f)));
    g.setGradientFill (ColourGradient (Colour (0xff3a3e45), c.x, c.y - kr, Colour (0xff17191d), c.x, c.y + kr, false));
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawEllipse (Rectangle<float> (kr * 2.0f - 1.0f, kr * 2.0f - 1.0f).withCentre (c), 1.0f);

    g.setColour (UiColours::accent);
    g.drawLine (Line<float> (c.getPointOnCircumference (kr * 0.3f, ang), c.getPointOnCircumference (kr * 0.85f, ang)), 2.5f);
}

// =============================================================================
//  CurveDisplay
// =============================================================================
CurveDisplay::CurveDisplay (MicMatchAudioProcessor& p, MeteoraLookAndFeel& l)
    : processor (p), lnf (l),
      lastSettings (p.readSettings()),
      stages (MicMatchEngine::computeStages (lastSettings))
{
    setInterceptsMouseClicks (false, false);
}

void CurveDisplay::refreshIfChanged()
{
    const auto s = processor.readSettings();
    if (s == lastSettings)
        return;

    lastSettings = s;
    stages = MicMatchEngine::computeStages (s);
    repaint();
}

void CurveDisplay::paint (Graphics& g)
{
    constexpr double fMin = 20.0, fMax = 20000.0;
    constexpr float range = 18.0f;

    // area del grafico, coordinate locali (nel mockup: x 226-641, y 34-176)
    const Rectangle<float> plot (36.0f, 17.0f, 415.0f, 142.0f);

    auto xFor = [&] (double f)
    {
        return plot.getX() + plot.getWidth() * (float) (std::log10 (f / fMin) / std::log10 (fMax / fMin));
    };
    auto yFor = [&] (double db)
    {
        return jmap ((float) jlimit (-(double) range, (double) range, db), -range, range, plot.getBottom(), plot.getY());
    };

    // griglia
    g.setColour (Colours::white.withAlpha (0.06f));
    for (double decade : { 10.0, 100.0, 1000.0, 10000.0 })
        for (int m = 1; m <= 9; ++m)
        {
            const double f = decade * m;
            if (f >= fMin && f <= fMax)
                g.drawVerticalLine (roundToInt (xFor (f)), plot.getY(), plot.getBottom());
        }
    for (float db : { -18.0f, -12.0f, -6.0f, 6.0f, 12.0f, 18.0f })
        g.drawHorizontalLine (roundToInt (yFor (db)), plot.getX(), plot.getRight());
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawHorizontalLine (roundToInt (yFor (0.0)), plot.getX(), plot.getRight());

    // etichette assi
    g.setFont (lnf.font (11.0f));
    g.setColour (UiColours::textDim);
    g.drawText ("+18dB", Rectangle<float> (0.0f, plot.getY() - 7.0f, 34.0f, 14.0f), Justification::centredRight);
    g.drawText ("0",     Rectangle<float> (0.0f, yFor (0.0) - 7.0f, 34.0f, 14.0f), Justification::centredRight);
    g.drawText ("-18dB", Rectangle<float> (0.0f, plot.getBottom() - 7.0f, 34.0f, 14.0f), Justification::centredRight);
    g.drawText ("dB",    Rectangle<float> (plot.getRight() + 4.0f, yFor (0.0) - 7.0f, 30.0f, 14.0f), Justification::centredLeft);

    const float labelY = plot.getBottom() + 6.0f;
    g.drawText ("20Hz", Rectangle<float> (40.0f, 14.0f).withCentre ({ xFor (20.0) + 4.0f, labelY + 7.0f }), Justification::centred);
    g.drawText ("200",  Rectangle<float> (40.0f, 14.0f).withCentre ({ xFor (200.0), labelY + 7.0f }), Justification::centred);
    g.drawText ("Frequency", Rectangle<float> (70.0f, 14.0f).withCentre ({ (xFor (200.0) + xFor (2000.0)) * 0.5f, labelY + 7.0f }), Justification::centred);
    g.drawText ("2k",   Rectangle<float> (40.0f, 14.0f).withCentre ({ xFor (2000.0), labelY + 7.0f }), Justification::centred);
    g.drawText ("20k",  Rectangle<float> (40.0f, 14.0f).withCentre ({ xFor (20000.0) - 4.0f, labelY + 7.0f }), Justification::centred);

    // curve: target (con gli switch) e sorgente corretta
    const double sr = processor.getProcessingSampleRate();

    MatchSettings targetSwitches;
    targetSwitches.match = 0.0f;
    targetSwitches.bassRolloff = lastSettings.bassRolloff;
    targetSwitches.presence = lastSettings.presence;
    const auto targetStages = MicMatchEngine::computeStages (targetSwitches);

    Path targetPath, corrected, targetFill;
    Point<float> nodePoint;
    double nodeDb = -100.0;
    constexpr int numPoints = 260;

    for (int i = 0; i <= numPoints; ++i)
    {
        const double f = MicProfiles::minHz * std::pow (MicProfiles::maxHz / MicProfiles::minHz, (double) i / numPoints);
        const double tDb = MicProfiles::interpolate (MicProfiles::target, f) + MicMatchEngine::magnitudeDb (targetStages, f, sr);
        const double cDb = MicProfiles::interpolate (MicProfiles::source, f) + MicMatchEngine::magnitudeDb (stages, f, sr);

        const Point<float> tp (xFor (f), yFor (tDb));
        const Point<float> cp (xFor (f), yFor (cDb));

        if (i == 0)
        {
            targetPath.startNewSubPath (tp);
            corrected.startNewSubPath (cp.x, plot.getBottom());
            targetFill.startNewSubPath (tp.x, plot.getBottom());
        }
        else
        {
            targetPath.lineTo (tp);
        }
        corrected.lineTo (cp);
        targetFill.lineTo (tp);

        if (f > 2000.0 && f < 9000.0 && tDb > nodeDb)
        {
            nodeDb = tDb;
            nodePoint = tp;
        }
    }

    const float rightX = xFor (MicProfiles::maxHz);
    targetFill.lineTo (rightX, plot.getBottom());
    targetFill.closeSubPath();
    corrected.lineTo (rightX, plot.getBottom());
    corrected.closeSubPath();

    g.setColour (Colours::white.withAlpha (0.07f));
    g.fillPath (targetFill);

    g.setGradientFill (ColourGradient (UiColours::teal.withAlpha (0.75f), 0.0f, plot.getY() + 40.0f,
                                       UiColours::teal.withAlpha (0.25f), 0.0f, plot.getBottom(), false));
    g.fillPath (corrected);
    g.setColour (UiColours::teal.brighter (0.3f));
    g.strokePath (corrected, PathStrokeType (1.2f));

    g.setColour (Colours::white.withAlpha (0.15f));
    g.strokePath (targetPath, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white);
    g.strokePath (targetPath, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    g.fillEllipse (Rectangle<float> (9.0f, 9.0f).withCentre (nodePoint));

    // intestazione
    g.setFont (lnf.font (13.0f, true));
    g.setColour (UiColours::text);
    g.drawText ("Target: SM7B", Rectangle<float> (40.0f, 4.0f, 200.0f, 16.0f), Justification::centredLeft);
    g.setFont (lnf.font (13.0f));
    g.setColour (UiColours::textDim);
    g.drawText ("Source: BC500", Rectangle<float> (40.0f, 19.0f, 200.0f, 16.0f), Justification::centredLeft);
    g.setColour (UiColours::teal);
    g.drawText ("Correction Profile", Rectangle<float> (40.0f, 34.0f, 200.0f, 16.0f), Justification::centredLeft);
}

// =============================================================================
//  ModeSelector e LedToggle
// =============================================================================
ModeSelector::ModeSelector (RangedAudioParameter& p)
    : attachment (p, [this] (float v) { index = roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void ModeSelector::paint (Graphics& g)
{
    drawLed (g, { 11.0f, 10.0f }, index == 0);
    drawLed (g, { 11.0f, 35.0f }, index == 1);
}

void ModeSelector::mouseDown (const MouseEvent& e)
{
    attachment.setValueAsCompleteGesture (e.position.y < (float) getHeight() * 0.5f ? 0.0f : 1.0f);
}

LedToggle::LedToggle (RangedAudioParameter& p, const String& text, MeteoraLookAndFeel& l)
    : attachment (p, [this] (float v) { on = v > 0.5f; repaint(); }),
      label (text), lnf (l)
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void LedToggle::paint (Graphics& g)
{
    drawLed (g, { 11.0f, (float) getHeight() * 0.5f }, on);
    g.setFont (lnf.font (14.0f));
    g.setColour (on ? UiColours::text : UiColours::textDim);
    g.drawText (label, getLocalBounds().withTrimmedLeft (28), Justification::centredLeft);
}

void LedToggle::mouseDown (const MouseEvent&)
{
    attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}

// =============================================================================
//  MeterPanel (coordinate locali: y 0 = 490 della finestra)
// =============================================================================
void MeterPanel::setLevels (float ip, float ir, float op, float orms)
{
    inPk = ip; inRms = ir; outPk = op; outRms = orms;
    outPkMax  = jmax (outPkMax, op);
    outRmsMax = jmax (outRmsMax, orms);
    repaint();
}

void MeterPanel::mouseDown (const MouseEvent&)
{
    outPkMax = outPk;
    outRmsMax = outRms;
    repaint();
}

void MeterPanel::paint (Graphics& g)
{
    constexpr float floorDb = -60.0f;

    auto drawBar = [&] (Rectangle<float> r, float db)
    {
        g.setColour (Colour (0xff23262b));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);

        const float norm = jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
        if (norm <= 0.0f)
            return;

        const auto fill = r.withWidth (jmax (r.getHeight(), r.getWidth() * norm));
        g.setGradientFill (ColourGradient (Colour (0xffb9bdc2), fill.getX(), fill.getY(),
                                           Colour (0xff8d9196), fill.getX(), fill.getBottom(), false));
        g.fillRoundedRectangle (fill, r.getHeight() * 0.5f);
        g.setColour (Colour (0xffd8dbde));
        g.fillRect (fill.withLeft (fill.getRight() - 10.0f).reduced (0.0f, 1.0f).withTrimmedRight (3.0f));
    };

    auto drawDots = [&] (float y, float db)
    {
        const float xs[]  = { 715.0f, 732.0f, 749.0f };
        const float thr[] = { -12.0f, -6.0f, -1.0f };
        for (int i = 0; i < 3; ++i)
        {
            const bool lit = db >= thr[i];
            const auto col = i == 2 ? Colour (0xffe5484d) : Colour (0xff8fe36b);
            const auto r = Rectangle<float> (7.0f, 7.0f).withCentre ({ xs[i], y });
            if (lit)
            {
                g.setColour (col.withAlpha (0.3f));
                g.fillEllipse (r.expanded (3.0f));
            }
            g.setColour (lit ? col : col.withAlpha (0.25f));
            g.fillEllipse (r);
        }
    };

    const float peakY = 45.5f, rmsY = 68.5f, barH = 9.0f;

    // Input
    drawBar ({ 70.0f, peakY - barH * 0.5f, 204.0f, barH }, inPk);
    drawBar ({ 70.0f, rmsY  - barH * 0.5f, 204.0f, barH }, inRms);

    g.setFont (lnf.font (16.0f));
    g.setColour (UiColours::text.withAlpha (0.85f));
    g.drawText ("Peak: " + dbText (inPk) + "dB", Rectangle<float> (286.0f, peakY - 10.0f, 140.0f, 20.0f), Justification::centredLeft);
    g.drawText ("RMS: "  + dbText (inRms) + "dB", Rectangle<float> (286.0f, rmsY - 10.0f, 140.0f, 20.0f), Justification::centredLeft);

    // Output: a sinistra il valore attuale, a destra il massimo (clic per azzerare)
    drawBar ({ 553.0f, peakY - barH * 0.5f, 219.0f, barH }, outPk);
    drawBar ({ 553.0f, rmsY  - barH * 0.5f, 219.0f, barH }, outRms);
    drawDots (peakY, outPk);
    drawDots (rmsY, outRms);

    g.drawText (dbText (outPk) + "dB",     Rectangle<float> (470.0f, peakY - 10.0f, 76.0f, 20.0f), Justification::centredRight);
    g.drawText (dbText (outRms) + "dB",    Rectangle<float> (470.0f, rmsY - 10.0f, 76.0f, 20.0f), Justification::centredRight);
    g.drawText (dbText (outPkMax) + "dB",  Rectangle<float> (780.0f, peakY - 10.0f, 80.0f, 20.0f), Justification::centredLeft);
    g.drawText (dbText (outRmsMax) + "dB", Rectangle<float> (780.0f, rmsY - 10.0f, 80.0f, 20.0f), Justification::centredLeft);
}

// =============================================================================
//  Editor
// =============================================================================
MicMatchAudioProcessorEditor::MicMatchAudioProcessorEditor (MicMatchAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      curve (p, lnf),
      modeSelector (*p.apvts.getParameter (ParamIDs::mode)),
      rolloffToggle  (*p.apvts.getParameter (ParamIDs::bassRolloff), "Bass roll-off", lnf),
      presenceToggle (*p.apvts.getParameter (ParamIDs::presence),    "Presence",      lnf),
      meters (lnf)
{
    setLookAndFeel (&lnf);
    background = ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);

    addAndMakeVisible (curve);

    setupKnob (matchKnob,  "big",   "%");
    setupKnob (trimKnob,   "dots",  " dB", true);
    setupKnob (outputKnob, "dots",  " dB");
    setupKnob (detailKnob, "small", "%");
    setupKnob (smoothKnob, "small", "%");

    addAndMakeVisible (modeSelector);
    addAndMakeVisible (rolloffToggle);
    addAndMakeVisible (presenceToggle);
    addAndMakeVisible (meters);

    auto& s = proc.apvts;
    matchAtt  = std::make_unique<SliderAttachment> (s, ParamIDs::match,       matchKnob);
    trimAtt   = std::make_unique<SliderAttachment> (s, ParamIDs::inputTrim,   trimKnob);
    outputAtt = std::make_unique<SliderAttachment> (s, ParamIDs::outputLevel, outputKnob);
    detailAtt = std::make_unique<SliderAttachment> (s, ParamIDs::detail,      detailKnob);
    smoothAtt = std::make_unique<SliderAttachment> (s, ParamIDs::smooth,      smoothKnob);

    // doppio clic: valore di default del parametro
    for (auto [slider, id] : { std::pair<Slider*, const char*> (&matchKnob,  ParamIDs::match),
                               std::pair<Slider*, const char*> (&trimKnob,   ParamIDs::inputTrim),
                               std::pair<Slider*, const char*> (&outputKnob, ParamIDs::outputLevel),
                               std::pair<Slider*, const char*> (&detailKnob, ParamIDs::detail),
                               std::pair<Slider*, const char*> (&smoothKnob, ParamIDs::smooth) })
    {
        auto* param = s.getParameter (id);
        slider->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

MicMatchAudioProcessorEditor::~MicMatchAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MicMatchAudioProcessorEditor::setupKnob (Slider& knob, const char* style, const String& suffix, bool bipolar)
{
    knob.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    knob.setTextValueSuffix (suffix);
    knob.getProperties().set ("knobStyle", style);
    knob.getProperties().set ("bipolar", bipolar);
    knob.setMouseDragSensitivity (220);
    knob.onValueChange = [this] { repaint(); };
    addAndMakeVisible (knob);
}

void MicMatchAudioProcessorEditor::paint (Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());
    else
        g.fillAll (Colour (0xff2b2f36));

    auto centredText = [&g] (const String& text, Point<float> centre, float width)
    {
        g.drawText (text, Rectangle<float> (width, 20.0f).withCentre (centre), Justification::centred);
    };

    // valori sotto Detail e Smooth
    g.setFont (lnf.font (11.0f));
    g.setColour (UiColours::textDim);
    centredText (detailKnob.getTextFromValue (detailKnob.getValue()), { 472.0f, 256.0f }, 60.0f);
    centredText (smoothKnob.getTextFromValue (smoothKnob.getValue()), { 619.0f, 256.0f }, 60.0f);

    // tacche di scala
    g.setFont (lnf.font (10.0f));
    centredText ("-6",   { 240.0f, 432.0f }, 30.0f);
    centredText ("+6",   { 290.0f, 432.0f }, 30.0f);
    centredText ("-inf", { 583.0f, 432.0f }, 30.0f);
    centredText ("+12",  { 626.0f, 432.0f }, 30.0f);

    g.setFont (lnf.font (15.0f));
    g.setColour (UiColours::text.withAlpha (0.85f));
    centredText ("0",    { 381.0f, 460.0f }, 30.0f);
    centredText ("100%", { 488.0f, 460.0f }, 50.0f);

    // valori di Input Trim e Output Level
    g.setFont (lnf.font (18.0f));
    g.setColour (UiColours::text);
    centredText (trimKnob.getTextFromValue (trimKnob.getValue()),     { 264.0f, 450.0f }, 110.0f);
    centredText (outputKnob.getTextFromValue (outputKnob.getValue()), { 604.0f, 450.0f }, 110.0f);
}

void MicMatchAudioProcessorEditor::resized()
{
    curve.setBounds (curveBounds);
    matchKnob.setBounds (matchBounds);
    trimKnob.setBounds (trimBounds);
    outputKnob.setBounds (outputBounds);
    detailKnob.setBounds (detailBounds);
    smoothKnob.setBounds (smoothBounds);
    modeSelector.setBounds (modeBounds);
    rolloffToggle.setBounds (708, 418, 140, 24);
    presenceToggle.setBounds (708, 446, 140, 24);
    meters.setBounds (meterBounds);
}

void MicMatchAudioProcessorEditor::timerCallback()
{
    curve.refreshIfChanged();

    auto update = [] (MicMatchAudioProcessor::Meter& m, float& peakDb, float& rmsDb)
    {
        const float newPeak = Decibels::gainToDecibels (m.peak.exchange (0.0f), -60.0f);
        peakDb = jmax (newPeak, peakDb - 1.5f);
        const float newRms = Decibels::gainToDecibels (m.rms.load(), -60.0f);
        rmsDb += 0.3f * (newRms - rmsDb);
    };

    update (proc.inputMeter,  inPeakDb,  inRmsDb);
    update (proc.outputMeter, outPeakDb, outRmsDb);

    meters.setLevels (inPeakDb, inRmsDb, outPeakDb, outRmsDb);
}
