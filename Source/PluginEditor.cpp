#include "PluginEditor.h"
#include "BinaryData.h"
#include "Combinacions.h"

namespace
{
    using escoita::U;
    using escoita::numero;
}

namespace ui
{
// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }

juce::String noteName (int midi)
{
    static const char* n[12] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
    return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2);
}

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour luz)
{
    if (on)
    {
        g.setColour (luz.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (luz.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (luz); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

// rótulo dun grupo: na súa cor e cunha liña debaixo
void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{
    g.setColour (c);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centred, 9.0f, 0.28f);
    g.setColour (c.withAlpha (0.25f));
    g.fillRect (r.getX() + 8.0f, r.getY() + 18.0f, r.getWidth() - 16.0f, 1.0f);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

// as teclas: en relevo, cun LED (aceso se está escollida; «led» = a cor da capa) e o texto en
// maiúsculas espaciadas; «gold» = un visor dourado (o selector da combinación)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
        return;
    }
    paintKey (g, r, over, down);
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed"))
    {
        const auto luz = b.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["led"]) : col::laranxa;
        paintLed (g, { r.getX() + 12.0f, r.getCentreY() }, b.getToggleState(), luz);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (12.0f, 0.1f));
        glowText (g, upper (b.getButtonText()), r.reduced (14.0f, 0.0f).withTrimmedRight (16.0f), juce::Justification::centredLeft, col::ouro);
        juce::Path arrow;
        const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (col::ouro); g.fillPath (arrow);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (14.0f);
    juce::Colour c = b.getToggleState() || over ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    const int curve = b.getProperties()["curve"];   // desfacer (−1) e refacer (+1): unha frecha en arco
    if (curve != 0)
    {
        const auto ctr = r.getCentre(); const float rad = 6.5f;
        const float a0 = curve > 0 ? -2.4f : 2.4f, a1 = curve > 0 ? 1.2f : -1.2f;
        juce::Path arc; arc.addCentredArc (ctr.x, ctr.y + 1.0f, rad, rad, 0.0f, a0, a1, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const juce::Point<float> tip (ctr.x + rad * std::sin (a1), ctr.y + 1.0f - rad * std::cos (a1));
        juce::Path head; head.addTriangle (tip.x + (float) curve * 4.0f, tip.y, tip.x - (float) curve * 1.5f, tip.y - 4.0f, tip.x - (float) curve * 1.5f, tip.y + 4.0f);
        g.fillPath (head);
        return;
    }
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

// os mandos: disco mecanizado, anel de LED laranxa e o valor nun visor debaixo
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    auto lcd = area.removeFromBottom (17.0f);
    lcd = lcd.withSizeKeepingCentre (juce::jmin (66.0f, area.getWidth() - 2.0f), 17.0f);
    area.removeFromBottom (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.withSizeKeepingCentre (d, d).getCentre();
    const float ring = d * 0.5f - 4.0f;
    const auto luz = s.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["led"]) : col::laranxa;

    juce::Path track; track.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
    const double zeroAt = bip ? 0.0 : s.getMinimum();
    const float zero = a0 + (float) s.valueToProportionOfLength (zeroAt) * (a1 - a0);
    const float ang = a0 + pos * (a1 - a0);
    if (std::abs (ang - zero) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, juce::jmin (zero, ang), juce::jmax (zero, ang), true);
        const auto lit = s.isEnabled() ? luz : col::crema3;
        g.setColour (lit.withAlpha (0.12f)); g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit.withAlpha (0.26f)); g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit); g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = ring - 7.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disc (juce::Colour (0xff4a5577), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disc.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disc);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);
    const juce::Point<float> tip (c.x + kr * 0.82f * std::sin (ang), c.y - kr * 0.82f * std::cos (ang));
    g.setColour (col::crema.withAlpha (s.isEnabled() ? 0.92f : 0.4f));
    g.drawLine ({ c, tip }, 2.0f);

    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.0f, 0.04f));
    glowText (g, upper (s.getTextFromValue (s.getValue())), lcd, juce::Justification::centred, s.isEnabled() ? col::ouro : col::crema3);
}

// os deslizadores: a pista afundida, a luz laranxa desde abaixo ata o capuchón e o capuchón mecanizado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);
    auto slot = juce::Rectangle<float> (cx - 4.5f, top - 7.0f, 9.0f, bot - top + 14.0f);
    paintWell (g, slot, 4.5f);
    const int steps = s.getMaximum() - s.getMinimum() <= 3.5 ? (int) (s.getMaximum() - s.getMinimum()) : 10;
    g.setColour (col::crema3.withAlpha (0.55f));
    for (int t = 0; t <= steps; ++t)
    {
        const float ty = bot - (bot - top) * (float) t / (float) juce::jmax (1, steps);
        const float len = (t % 5 == 0 || steps <= 3) ? 5.0f : 2.5f;
        g.fillRect (cx - 9.0f - len, ty - 0.5f, len, 1.0f);
        g.fillRect (cx + 9.0f, ty - 0.5f, len, 1.0f);
    }
    if (pos < bot - 0.5f)
    {
        auto lit = juce::Rectangle<float> (cx - 1.5f, pos, 3.0f, bot + 4.0f - pos);
        g.setColour (col::laranxa.withAlpha (0.14f)); g.fillRoundedRectangle (lit.expanded (3.0f, 0.0f), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.30f)); g.fillRoundedRectangle (lit.expanded (1.5f, 0.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }
    auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 7.0f, 26.0f, 14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (cap.translated (0.0f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), cap.getX(), cap.getY(), juce::Colour (0xff141a2e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::crema); g.fillRect (cap.getX() + 4.0f, pos - 0.8f, cap.getWidth() - 8.0f, 1.6f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

// ================================================================ mando, deslizador e número
Knob::Knob()
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (slider.isEnabled() ? col::crema2 : col::crema3);
    drawCaps (g, name, getLocalBounds().removeFromBottom (12).toFloat(), juce::Justification::centred, 7.5f, 0.16f);
}

Fader::Fader()
{
    slider.setSliderSnapsToMousePosition (false);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (200);
    addAndMakeVisible (slider);
}

void Fader::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (readoutH + labelH + 6).withTrimmedTop (8)); }

void Fader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto label = r.removeFromBottom ((float) labelH);
    r.removeFromBottom (3.0f);
    auto lcd = r.removeFromBottom ((float) readoutH).withSizeKeepingCentre (juce::jmin (60.0f, r.getWidth()), (float) readoutH);
    paintWell (g, lcd, 5.0f);
    g.setFont (caps (8.5f, 0.02f));
    glowText (g, upper (slider.getTextFromValue (slider.getValue())), lcd, juce::Justification::centred, col::ouro);
    g.setColour (col::crema2);
    drawCaps (g, name, label, juce::Justification::centred, 8.0f, 0.16f);
}

void NumBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    if (name.isNotEmpty())
    {
        g.setColour (col::crema3);
        drawCaps (g, name, r.removeFromTop (12.0f), juce::Justification::centred, 7.0f, 0.1f);
    }
    paintWell (g, r.reduced (1.0f, 0.0f), 5.0f);
    g.setFont (caps (10.0f, 0.02f));
    glowText (g, upper (text ? text (value) : juce::String (value)), r, juce::Justification::centred, isEnabled() ? tint : col::crema3);
}

void NumBox::mouseDown (const juce::MouseEvent&) { startValue = value; }

void NumBox::mouseDrag (const juce::MouseEvent& e)
{
    // cara arriba sube; os rangos pequenos van máis amodo, para non saltar valores
    const float porPaso = max - min <= 15 ? 9.0f : max - min <= 99 ? 3.0f : 2.0f;
    setValue (startValue + juce::roundToInt (-(float) e.getDistanceFromDragStartY() / porPaso), true);
}

void NumBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    wheelAcc += w.deltaY * (w.isReversed ? -1.0f : 1.0f) * 12.0f;
    const int pasos = (int) wheelAcc;
    if (pasos != 0) { wheelAcc -= (float) pasos; setValue (value + pasos, true); }
}

void NumBox::setValue (int v, bool notify)
{
    v = juce::jlimit (min, max, v);
    if (v == value) return;
    value = v;
    repaint();
    if (notify && onChange) onChange (v);
}

// ================================================================ o mapa do teclado
juce::Rectangle<float> ZoneMap::keys() const { return getLocalBounds().toFloat().reduced (2.0f, 0.0f).removeFromBottom (24.0f); }
float ZoneMap::xOf (int note) const { const auto k = keys(); return k.getX() + k.getWidth() * (float) note / 128.0f; }
int ZoneMap::noteAt (float x) const { const auto k = keys(); return juce::jlimit (0, 127, (int) ((x - k.getX()) / k.getWidth() * 128.0f)); }

void ZoneMap::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const auto kb = keys();
    // as catro capas, unha fila cada unha
    const float rowH = (kb.getY() - r.getY() - 6.0f) / (float) BXTProcessor::CAPAS;
    for (int k = 0; k < BXTProcessor::CAPAS; ++k)
    {
        const auto row = juce::Rectangle<float> (kb.getX(), r.getY() + (float) k * rowH, kb.getWidth(), rowH - 3.0f);
        const bool on = proc.capaAcesa (k);
        auto& a = proc.apvts;
        const int lo = (int) a.getRawParameterValue (BXTProcessor::id (k, "notaBaixa"))->load();
        const int hi = (int) a.getRawParameterValue (BXTProcessor::id (k, "notaAlta"))->load();
        g.setColour (col::visor.withAlpha (0.6f)); g.fillRoundedRectangle (row, 3.0f);
        if (hi >= lo)
        {
            const auto bar = juce::Rectangle<float> (xOf (lo), row.getY(), xOf (hi + 1) - xOf (lo), row.getHeight());
            const auto c = col::capa (k);
            g.setColour (c.withAlpha (on ? 0.30f : 0.08f)); g.fillRoundedRectangle (bar, 3.0f);
            g.setColour (c.withAlpha (on ? 0.95f : 0.3f)); g.drawRoundedRectangle (bar.reduced (0.5f), 3.0f, 1.0f);
            g.fillRect (bar.getX(), bar.getY() + 2.0f, 2.5f, bar.getHeight() - 4.0f);
            g.fillRect (bar.getRight() - 2.5f, bar.getY() + 2.0f, 2.5f, bar.getHeight() - 4.0f);
            g.setColour (on ? col::crema : col::crema3);
            g.setFont (caps (8.0f, 0.1f));
            const auto t = juce::String (k + 1) + "  " + upper (proc.nomeSon (k, proc.tipoCapa (k)));
            g.drawText (t, bar.reduced (7.0f, 0.0f), juce::Justification::centredLeft, true);
        }
    }
    // o teclado (as 128 notas), coas que soan acesas
    g.setColour (juce::Colour (0xffe9e7e0)); g.fillRoundedRectangle (kb, 2.0f);
    for (int n = 0; n < 128; ++n)
    {
        const int pc = n % 12;
        const bool negra = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        const bool soa = proc.keyboardState.isNoteOnForChannels (0xffff, n);
        const float x0 = xOf (n), x1 = xOf (n + 1);
        if (negra) { g.setColour (soa ? col::laranxa.darker (0.2f) : juce::Colour (0xff1a1b1e)); g.fillRect (x0, kb.getY(), x1 - x0, kb.getHeight() * 0.6f); }
        else if (soa) { g.setColour (col::laranxa); g.fillRect (x0, kb.getY(), x1 - x0, kb.getHeight()); }
        if (pc == 0)
        {
            g.setColour (juce::Colour (0xff8a877f)); g.fillRect (x0, kb.getY() + kb.getHeight() * 0.65f, 0.8f, kb.getHeight() * 0.35f);
            if (n % 24 == 0 || n == 60) { g.setFont (caps (6.5f, 0.0f)); g.drawText (noteName (n), juce::Rectangle<float> (x0 + 1.5f, kb.getBottom() - 9.0f, 30.0f, 9.0f), juce::Justification::centredLeft, false); }
        }
    }
}

void ZoneMap::mouseDown (const juce::MouseEvent& e)
{
    // o extremo máis próximo dunha capa acesa, na súa fila
    dragLayer = -1;
    const auto kb = keys();
    const float rowH = (kb.getY() - 6.0f) / (float) BXTProcessor::CAPAS;
    const int k = juce::jlimit (0, BXTProcessor::CAPAS - 1, (int) (e.position.y / rowH));
    if (e.position.y > kb.getY()) return;
    auto& a = proc.apvts;
    const int lo = (int) a.getRawParameterValue (BXTProcessor::id (k, "notaBaixa"))->load();
    const int hi = (int) a.getRawParameterValue (BXTProcessor::id (k, "notaAlta"))->load();
    dragLayer = k;
    dragHigh = std::abs (e.position.x - xOf (hi + 1)) < std::abs (e.position.x - xOf (lo));
    if (auto* p = a.getParameter (BXTProcessor::id (k, dragHigh ? "notaAlta" : "notaBaixa"))) p->beginChangeGesture();
    mouseDrag (e);
}

void ZoneMap::mouseDrag (const juce::MouseEvent& e)
{
    if (dragLayer < 0) return;
    auto& a = proc.apvts;
    const int lo = (int) a.getRawParameterValue (BXTProcessor::id (dragLayer, "notaBaixa"))->load();
    const int hi = (int) a.getRawParameterValue (BXTProcessor::id (dragLayer, "notaAlta"))->load();
    int n = noteAt (e.position.x);
    n = dragHigh ? juce::jmax (lo, n) : juce::jmin (hi, n);
    if (auto* p = a.getParameter (BXTProcessor::id (dragLayer, dragHigh ? "notaAlta" : "notaBaixa"))) p->setValueNotifyingHost (p->convertTo0to1 ((float) n));
    setTooltip ((dragHigh ? U ("Nota alta: ") : U ("Nota baixa: ")) + noteName (n));
    repaint();
}

void ZoneMap::mouseUp (const juce::MouseEvent&)
{
    if (dragLayer >= 0)
        if (auto* p = proc.apvts.getParameter (BXTProcessor::id (dragLayer, dragHigh ? "notaAlta" : "notaBaixa"))) p->endChangeGesture();
    dragLayer = -1;
}

// ================================================================ unha capa
LayerStrip::LayerStrip (BXTProcessor& p, MainView& m, int index) : proc (p), main (m), k (index)
{
    const auto cor = (juce::int64) col::capa (k).getARGB();
    auto key = [this] (juce::TextButton& b, const juce::String& tip) { b.setTooltip (tip); b.setWantsKeyboardFocus (false); addAndMakeVisible (b); };
    auto& a = proc.apvts;

    onBtn.setButtonText (U ("Capa ") + juce::String (k + 1));
    onBtn.getProperties().set ("led", cor);
    {
        auto* prm = a.getParameter (BXTProcessor::id (k, "on"));
        onAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) { onBtn.setToggleState (v > 0.5f, juce::dontSendNotification); repaint(); });
        onAtt->sendInitialUpdate();
        onBtn.onClick = [this] { onAtt->setValueAsCompleteGesture (onBtn.getToggleState() ? 0.0f : 1.0f); };
    }
    key (onBtn, U ("Acende ou apaga esta capa."));
    const char* motores[3] = { "Xuno", "7FM", "FM" };
    const juce::String axudas[3] = { U ("Xuno: o Juno-106 de Barullo Xuno (sustractiva)."), U ("7FM: o DX7 de Barullo 7FM (FM de seis operadores)."),
                                     U ("FM: o chip de dous operadores do teclado de Barullo FM.") };
    {
        auto* prm = a.getParameter (BXTProcessor::id (k, "motor"));
        engineAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this, prm] (float v)
        {
            const int t = (int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v)));
            for (int i = 0; i < 3; ++i) engineBtn[i].setToggleState (i == t, juce::dontSendNotification);
            refresh();
        });
        for (int i = 0; i < 3; ++i)
        {
            engineBtn[i].setButtonText (motores[i]);
            engineBtn[i].getProperties().set ("led", cor);
            engineBtn[i].onClick = [this, i] { engineAtt->setValueAsCompleteGesture ((float) i); };
            key (engineBtn[i], axudas[i]);
        }
        engineAtt->sendInitialUpdate();
    }
    for (auto* b : { &prevBtn, &nextBtn, &soundsBtn, &editBtn }) b->getProperties().set ("noLed", true);
    prevBtn.onClick = [this] { proc.pasoSon (k, -1); };
    nextBtn.onClick = [this] { proc.pasoSon (k, 1); };
    soundsBtn.onClick = [this] { soundsMenu(); };
    editBtn.onClick = [this] { main.openEditor (k); };
    key (prevBtn, U ("O son anterior do banco."));
    key (nextBtn, U ("O son seguinte do banco."));
    key (soundsBtn, U ("Os bancos e os sons deste motor; abrir un .syx; volver ao son orixinal."));
    key (editBtn, U ("Editar o son desta capa: os parámetros do seu motor, que soan ao momento."));

    auto knob = [this, cor] (Knob& kn, const char* pid, const juce::String& name, const juce::String& tip, std::function<juce::String (double)> fmt)
    {
        kn.name = name;
        kn.slider.setTooltip (tip);
        kn.slider.getProperties().set ("led", cor);
        kn.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, BXTProcessor::id (k, pid), kn.slider);
        kn.slider.textFromValueFunction = std::move (fmt);
        addAndMakeVisible (kn);
    };
    knob (volume, "volume", "Volume", U ("O volume da capa."), [] (double v) { return v <= -59.9 ? U ("−∞") : (v >= 0.05 ? "+" : "") + numero (v, 1) + " dB"; });
    knob (pan, "pan", "Pan", U ("Panorama: esquerda ou dereita."), [] (double v) { const int x = juce::roundToInt (v * 100); return x == 0 ? juce::String ("C") : (x < 0 ? "E " : "D ") + juce::String (std::abs (x)); });
    knob (rev, "reverb", "Reverb", U ("Canto manda esta capa á reverb."), [] (double v) { return juce::String (juce::roundToInt (v * 100)) + " %"; });
    knob (eco, "eco", "Eco", U ("Canto manda esta capa ao eco."), [] (double v) { return juce::String (juce::roundToInt (v * 100)) + " %"; });
    knob (oct, "oitava", "Oitava", U ("Sobe ou baixa a capa por oitavas."), [] (double v) { return (v > 0 ? "+" : "") + numero (v, 0); });
    knob (transp, "semitons", U ("Semitóns"), U ("Transpón a capa por semitóns."), [] (double v) { return (v > 0 ? "+" : "") + numero (v, 0); });
    knob (cents, "cents", "Fino", U ("Afinación fina: unhas poucas cents de diferenza entre capas abren o son."), [] (double v) { return (v > 0.05 ? "+" : "") + numero (v, 1) + " c"; });
    knob (bend, "bend", "Bend", U ("Canto move a roda de afinación o ton desta capa."), [] (double v) { return U ("±") + juce::String ((int) v) + " st"; });
    for (auto* kn : { &volume, &pan, &cents }) kn->slider.setDoubleClickReturnValue (true, 0.0);

    // a zona: notas e velocidades
    auto zona = [this, &a] (NumBox& nb, const char* pid, const juce::String& name, const juce::String& tip, bool nota,
                            std::unique_ptr<juce::ParameterAttachment>& att)
    {
        auto* prm = a.getParameter (BXTProcessor::id (k, pid));
        nb.name = name; nb.setTooltip (tip);
        nb.min = nota ? 0 : 1; nb.max = 127; nb.tint = col::capa (k);
        if (nota) nb.text = [] (int v) { return noteName (v); };
        att = std::make_unique<juce::ParameterAttachment> (*prm, [&nb, prm] (float v) { nb.setValue ((int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v))), false); });
        auto* at = att.get();
        nb.onChange = [at] (int v) { at->setValueAsCompleteGesture ((float) v); };
        att->sendInitialUpdate();
        addAndMakeVisible (nb);
    };
    zona (noteLo, "notaBaixa", "Desde", U ("A nota máis baixa que toca esta capa (arrastra cara arriba ou abaixo; tamén no mapa)."), true, noteLoAtt);
    zona (noteHi, "notaAlta", "Ata", U ("A nota máis alta que toca esta capa."), true, noteHiAtt);
    zona (velLo, "velBaixa", "Vel. min", U ("A capa só soa coas notas tocadas con esta forza ou máis."), false, velLoAtt);
    zona (velHi, "velAlta", "Vel. max", U ("…e ata esta."), false, velHiAtt);
    {
        auto* prm = a.getParameter (BXTProcessor::id (k, "canle"));
        chanAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this, prm] (float v)
        {
            const int c = (int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v)));
            chanBtn.setButtonText (c == 0 ? U ("MIDI: todas") : U ("MIDI: ") + juce::String (c));
        });
        chanAtt->sendInitialUpdate();
        chanBtn.getProperties().set ("noLed", true);
        chanBtn.onClick = [this]
        {
            juce::PopupMenu menu;
            menu.addSectionHeader (U ("Canle MIDI desta capa"));
            const int cur = (int) std::lround (proc.apvts.getRawParameterValue (BXTProcessor::id (k, "canle"))->load());
            menu.addItem (100, U ("Todas"), true, cur == 0);
            for (int c = 1; c <= 16; ++c) menu.addItem (100 + c, juce::String (c), true, cur == c);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&chanBtn), [this] (int r) { if (r >= 100) chanAtt->setValueAsCompleteGesture ((float) (r - 100)); });
        };
        key (chanBtn, U ("A canle MIDI que escoita esta capa (todas ou unha: para tocar cada capa desde un teclado)."));
    }
    refresh();
}

void LayerStrip::soundsMenu()
{
    const int t = proc.tipoCapa (k);
    const auto& bancos = proc.bancos (t);
    juce::PopupMenu m;
    for (int b = 0; b < (int) bancos.size(); ++b)
    {
        juce::PopupMenu sub;
        const auto& bk = bancos[(size_t) b];
        const bool este = proc.bancoSon (k, t) == bk.nome;
        for (int i = 0; i < (int) bk.sons.size(); ++i)
            sub.addItem (1000 * (b + 1) + i, bk.nomes[i], true, este && proc.nomeSon (k, t) == bk.nomes[i]);
        m.addSubMenu (bk.nome + U (" — ") + bk.titulo, sub, true, nullptr, este);
    }
    m.addSeparator();
    if (xuntos::fabrica (t).lerSyx != nullptr)
        m.addItem (1, t == xuntos::XUNO ? U ("Abrir un .syx de Juno-106…") : U ("Abrir un .syx de DX7…"));
    m.addItem (2, U ("Volver ao son orixinal"), proc.editado (k, t));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&soundsBtn), [this, t] (int r)
    {
        if (r >= 1000) { proc.escollerSon (k, t, r / 1000 - 1, r % 1000); return; }
        if (r == 2) { proc.restaurarSon (k, t); return; }
        if (r != 1) return;
        chooser = std::make_unique<juce::FileChooser> (U ("Abrir un .syx"), juce::File(), "*.syx;*.SYX");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, t] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            juce::String msg;
            if (proc.abrirSyx (t, f, msg))
            {
                const auto& bs = proc.bancos (t);
                for (int b = 0; b < (int) bs.size(); ++b)   // o banco aberto: o seu primeiro son
                    if (bs[(size_t) b].nome == bs.back().nome) { proc.escollerSon (k, t, b, 0); break; }
            }
            main.showMessage (msg);
        });
    });
}

void LayerStrip::refresh()
{
    const bool on = proc.capaAcesa (k);
    for (auto* c : std::initializer_list<juce::Component*> { &engineBtn[0], &engineBtn[1], &engineBtn[2], &prevBtn, &nextBtn, &soundsBtn, &editBtn,
                                                             &volume, &pan, &rev, &eco, &oct, &transp, &cents, &bend, &noteLo, &noteHi, &velLo, &velHi, &chanBtn })
        c->setAlpha (on ? 1.0f : 0.45f);
    repaint();
}

void LayerStrip::tick()
{
    const float l = proc.nivelCapa[(size_t) k].load();
    const int v = proc.vocesCapa[(size_t) k].load();
    level = juce::jmax (l, level * 0.86f);
    if (std::abs (level - l) > 0.003f || v != voices || level > 0.001f) { voices = v; repaint (soundBox.expanded (2)); }
}

void LayerStrip::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    paintPanel (g, r, 14.0f);
    const auto c = col::capa (k);
    g.setColour (c.withAlpha (proc.capaAcesa (k) ? 0.9f : 0.3f));
    g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 3.0f, r.getWidth() - 28.0f, 2.0f, 1.0f);

    // a pantalla do son
    const auto sb = soundBox.toFloat();
    paintGlass (g, sb, 10.0f);
    const int t = proc.tipoCapa (k);
    auto in = sb.reduced (12.0f, 9.0f);
    g.setColour (c.withAlpha (0.9f));
    drawCaps (g, MainView::origin (proc, k, t), in.removeFromTop (12.0f), juce::Justification::centredLeft, 8.0f, 0.22f);
    if (proc.editado (k, t))
    {
        g.setColour (col::laranxa);
        drawCaps (g, "Editado", sb.reduced (12.0f, 9.0f).removeFromTop (12.0f), juce::Justification::centredRight, 8.0f, 0.2f);
    }
    in.removeFromTop (3.0f);
    g.setColour (col::crema);
    g.setFont (semi (16.0f));
    g.drawFittedText (upper (proc.nomeSon (k, t)), in.removeFromTop (20.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);
    in.removeFromTop (2.0f);
    g.setColour (col::crema2);
    g.setFont (body (11.5f));
    auto meter = in.removeFromBottom (6.0f);
    g.drawFittedText (proc.descricionSon (k, t), in.toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
    // as voces e o nivel
    paintWell (g, meter, 3.0f);
    const float lv = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels (level, -60.0f) / 60.0f + 1.0f);
    if (lv > 0.0f) { g.setColour (c.withAlpha (0.85f)); g.fillRoundedRectangle (meter.withWidth (meter.getWidth() * lv).reduced (1.0f), 2.0f); }
    g.setColour (col::crema3);
    drawCaps (g, voices > 0 ? juce::String (voices) + " v" : juce::String(), sb.reduced (12.0f, 6.0f).removeFromBottom (12.0f).withTrimmedBottom (8.0f),
              juce::Justification::bottomRight, 7.5f, 0.1f);

    // os rótulos das filas
    g.setColour (col::crema3);
    drawCaps (g, U ("Zona"), juce::Rectangle<float> (r.getX(), (float) noteLo.getY() - 14.0f, r.getWidth(), 10.0f), juce::Justification::centred, 7.0f, 0.3f);
}

void LayerStrip::resized()
{
    auto r = getLocalBounds().reduced (12, 0);
    r.removeFromTop (12);
    auto top = r.removeFromTop (28);
    chanBtn.setBounds (top.removeFromRight (104));
    top.removeFromRight (6);
    onBtn.setBounds (top);
    r.removeFromTop (6);
    auto eng = r.removeFromTop (28);
    const int ew = (eng.getWidth() - 8) / 3;
    for (int i = 0; i < 3; ++i) engineBtn[i].setBounds (eng.getX() + i * (ew + 4), eng.getY(), ew, 28);
    r.removeFromTop (8);
    soundBox = r.removeFromTop (96);
    r.removeFromTop (6);
    auto nav = r.removeFromTop (28);
    prevBtn.setBounds (nav.removeFromLeft (34)); nav.removeFromLeft (4);
    nextBtn.setBounds (nav.removeFromLeft (34)); nav.removeFromLeft (8);
    const int hw = (nav.getWidth() - 4) / 2;
    soundsBtn.setBounds (nav.removeFromLeft (hw)); nav.removeFromLeft (4);
    editBtn.setBounds (nav);
    r.removeFromTop (8);
    auto row = [&r] (std::initializer_list<Knob*> ks)
    {
        auto a = r.removeFromTop (76);
        const int w = a.getWidth() / (int) ks.size();
        int i = 0;
        for (auto* kn : ks) kn->setBounds (a.getX() + i++ * w, a.getY(), w, 76);
    };
    row ({ &volume, &pan, &rev, &eco });
    r.removeFromTop (4);
    row ({ &oct, &transp, &cents, &bend });
    r.removeFromTop (18);
    auto z = r.removeFromTop (34);
    const int zw = (z.getWidth() - 3 * 6) / 4;
    int i = 0;
    for (auto* nb : { &noteLo, &noteHi, &velLo, &velHi }) nb->setBounds (z.getX() + i++ * (zw + 6), z.getY(), zw, 34);
}

// ================================================================ vista principal
MainView::MainView (BXTProcessor& p) : proc (p), zones (p), editor (p, *this)
{
    setLookAndFeel (&look);
    auto key = [this] (juce::TextButton& b, const juce::String& tip) { b.setTooltip (tip); b.setWantsKeyboardFocus (false); addAndMakeVisible (b); };

    // cabeceira: a combinación, os presets, o A/B e desfacer
    combBtn.getProperties().set ("gold", true);
    combBtn.onClick = [this] { combinationsMenu(); };
    key (combBtn, U ("As combinacións que veñen co plugin: capas xa mesturadas e repartidas polo teclado."));
    prevComb.setButtonText (juce::String::fromUTF8 ("‹")); nextComb.setButtonText (juce::String::fromUTF8 ("›"));
    for (auto* b : { &prevComb, &nextComb, &presetsBtn, &undoBtn, &redoBtn, &funcBtn }) b->getProperties().set ("noLed", true);
    prevComb.onClick = [this] { proc.estado.novoPaso(); proc.cargarCombinacion ((proc.getCombinacion() + xuntos::NUM_COMBINACIONS - 1) % xuntos::NUM_COMBINACIONS); refreshAll(); };
    nextComb.onClick = [this] { proc.estado.novoPaso(); proc.cargarCombinacion ((proc.getCombinacion() + 1) % xuntos::NUM_COMBINACIONS); refreshAll(); };
    key (prevComb, U ("A combinación anterior."));
    key (nextComb, U ("A combinación seguinte."));
    presetsBtn.onClick = [this] { showPresetsMenu(); };
    key (presetsBtn, U ("Os teus presets: gardan todo (as capas, os seus sons, editados ou non, e os efectos). Aquí tamén: abrir a carpeta e copiar entre A e B."));
    for (int i = 0; i < 2; ++i)
    {
        abBtn[i].setButtonText (i == 0 ? "A" : "B");
        abBtn[i].onClick = [this, i] { if (i != proc.estado.lado()) proc.estado.cambiarLado(); refreshAll(); };
        key (abBtn[i], U ("A/B: dous xogos de axustes para comparar. Cambiar de lado garda o actual; para copiar un lado no outro, no menú dos presets."));
    }
    undoBtn.getProperties().set ("curve", -1); redoBtn.getProperties().set ("curve", 1);
    undoBtn.setTitle (U ("Desfacer")); redoBtn.setTitle (U ("Refacer"));
    undoBtn.onClick = [this] { proc.undo.undo(); refreshAll(); };
    redoBtn.onClick = [this] { proc.undo.redo(); refreshAll(); };
    key (undoBtn, U ("Desfacer o último cambio (un mando, un son, unha edición, unha combinación). Cada clic é un paso."));
    key (redoBtn, U ("Refacer."));
    funcBtn.setButtonText (U ("Función"));
    funcBtn.onClick = [this] { functionMenu(); };
    key (funcBtn, U ("Avisos e ligazóns."));

    for (int k = 0; k < BXTProcessor::CAPAS; ++k) addAndMakeVisible (strips.add (new LayerStrip (proc, *this, k)));
    addAndMakeVisible (zones);
    zones.setTooltip (U ("A zona do teclado de cada capa: arrastra os extremos para partilo. As teclas que soan acéndense."));

    // os efectos e o xeral
    auto knob = [this] (Knob& kn, const char* pid, const juce::String& name, const juce::String& tip, std::function<juce::String (double)> fmt)
    {
        kn.name = name;
        kn.slider.setTooltip (tip);
        kn.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, pid, kn.slider);
        kn.slider.textFromValueFunction = std::move (fmt);
        addAndMakeVisible (kn);
    };
    auto pc = [] (double v) { return juce::String (juce::roundToInt (v * 100)) + " %"; };
    knob (revSize, "revTamano", U ("Tamaño"), U ("O tempo que tarda a reverb en apagarse (de 0,3 a 10 s)."),
          [] (double v) { const double s = 0.3 * std::pow (10.0 / 0.3, v); return numero (s, s < 10 ? 1 : 0) + " s"; });
    knob (revBright, "revBrillo", "Brillo", U ("Canto brillo queda na cola."), pc);
    knob (revPre, "revPredelay", "Predelay", U ("O tempo antes de que chegue a reverb."), [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    knob (revVol, "revVolume", "Volume", U ("O volume da reverb."), pc);
    knob (ecoTime, "ecoMs", "Tempo", U ("O tempo do eco, cando o ritmo é «Libre»."), [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    knob (ecoFb, "ecoRealimentacion", "Repet.", U ("Cantas repeticións: a realimentación."), pc);
    knob (ecoTone, "ecoTon", "Ton", U ("O brillo das repeticións."), pc);
    knob (ecoVol, "ecoVolume", "Volume", U ("O volume do eco."), pc);
    knob (volume, "volume", "Volume", U ("O volume de saída (despois, o limitador a −1 dBFS)."), [] (double v) { return (v >= 0.05 ? "+" : "") + numero (v, 1) + " dB"; });
    volume.slider.setDoubleClickReturnValue (true, 0.0);
    {
        auto* prm = proc.apvts.getParameter ("ecoRitmo");
        ritmoAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this, prm] (float v)
        {
            const int r = (int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v)));
            ritmoBtn.setButtonText (U ("Ritmo: ") + prm->getText (prm->convertTo0to1 ((float) r), 16));
            ecoTime.slider.setEnabled (r == 0);
            ecoTime.repaint();
        });
        ritmoAtt->sendInitialUpdate();
        ritmoBtn.getProperties().set ("noLed", true);
        ritmoBtn.onClick = [this, prm]
        {
            juce::PopupMenu m;
            m.addSectionHeader (U ("O eco, á par do tempo do DAW"));
            const auto& ch = dynamic_cast<juce::AudioParameterChoice&> (*prm);
            for (int i = 0; i < ch.choices.size(); ++i) m.addItem (100 + i, ch.choices[i], true, i == ch.getIndex());
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&ritmoBtn), [this] (int r) { if (r >= 100) ritmoAtt->setValueAsCompleteGesture ((float) (r - 100)); });
        };
        key (ritmoBtn, U ("O tempo do eco: unha figura á par do tempo do DAW (1/8. é unha corchea con punto; T, tresillo) ou libre, en ms."));
    }
    {
        auto* prm = proc.apvts.getParameter ("ecoCruzado");
        cruzAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) { cruzBtn.setToggleState (v > 0.5f, juce::dontSendNotification); });
        cruzAtt->sendInitialUpdate();
        cruzBtn.onClick = [this] { cruzAtt->setValueAsCompleteGesture (cruzBtn.getToggleState() ? 0.0f : 1.0f); };
        key (cruzBtn, U ("Cruzado: as repeticións van dun lado ao outro."));
    }
    listenBtn.onStateChange = [this]
    {
        const bool down = listenBtn.isDown();
        if (down == listening) return;
        listening = down;
        for (int n : { 36, 48, 55, 60, 64, 67, 72 })
            if (down) proc.keyboardState.noteOn (1, n, 0.8f);
            else proc.keyboardState.noteOff (1, n, 0.0f);
    };
    key (listenBtn, U ("Mentres o tes premido, soa un acorde de Do en todo o teclado (para oír as capas e as zonas sen teclado MIDI)."));

    newVersion.getProperties().set ("noLed", true);
    newVersion.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    newVersion.setWantsKeyboardFocus (false);
    addChildComponent (newVersion);

    addChildComponent (editor);
    juce::Desktop::getInstance().addGlobalMouseListener (this);
    setSize (W, H);
    seenChanges = proc.cambios.load();
    refreshAll();
    startTimerHz (30);
}

MainView::~MainView()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

// cada clic (en calquera parte) empeza un paso novo de desfacer
void MainView::mouseDown (const juce::MouseEvent&) { proc.estado.novoPaso(); }

void MainView::openEditor (int layer)
{
    editor.open (layer);
    editor.setVisible (true);
    editor.toFront (false);
}

void MainView::closeEditor() { editor.setVisible (false); }

void MainView::combinationsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Combinacións"));
    for (int i = 0; i < xuntos::NUM_COMBINACIONS; ++i)
        m.addItem (100 + i, U (xuntos::COMBINACIONS[i].nome), true, i == proc.getCombinacion());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&combBtn), [this] (int r)
    {
        if (r < 100) return;
        proc.estado.novoPaso();
        proc.cargarCombinacion (r - 100);
        refreshAll();
    });
}

void MainView::showPresetsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Presets propios"));
    proc.estado.engadirAoMenu (m, [sp = juce::Component::SafePointer<MainView> (this)] { if (sp != nullptr) sp->refreshAll(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetsBtn));
}

void MainView::functionMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Función"));
    m.addItem (6, U ("Avisar das versións novas"), true, versions->enabled());
    m.addItem (7, U ("escoita.link"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&funcBtn), [this] (int r)
    {
        if (r == 6) versions->setEnabled (! versions->enabled());
        else if (r == 7) juce::URL ("https://www.escoita.link").launchInDefaultBrowser();
    });
}

void MainView::refreshState()
{
    for (int i = 0; i < 2; ++i)
        if (abBtn[i].getToggleState() != (i == proc.estado.lado())) abBtn[i].setToggleState (i == proc.estado.lado(), juce::dontSendNotification);
    if (undoBtn.isEnabled() != proc.undo.canUndo()) undoBtn.setEnabled (proc.undo.canUndo());
    if (redoBtn.isEnabled() != proc.undo.canRedo()) redoBtn.setEnabled (proc.undo.canRedo());
}

void MainView::refreshAll()
{
    combBtn.setButtonText (proc.nomeCombinacion());
    for (auto* s : strips) s->refresh();
    if (editor.isVisible()) editor.refresh();
    refreshState();
    zones.repaint();
    repaint (screenArea);
}

void MainView::timerCallback()
{
    const auto ch = proc.cambios.load();
    if (ch != seenChanges) { seenChanges = ch; refreshAll(); }
    else if (editor.isVisible() && editor.engine() != proc.tipoCapa (editor.layer())) editor.refresh();   // a capa cambiou de motor
    refreshState();
    for (auto* s : strips) s->tick();
    zones.repaint();
    repaint (screenArea.withTrimmedRight (zones.getWidth() + 30));
    if (message.isNotEmpty() && juce::Time::getMillisecondCounter() - messageTime > 4000) { message.clear(); repaint (screenArea); }

    const auto nv = versions->newVersion();
    if (nv.isNotEmpty() != newVersion.isVisible())
    {
        newVersion.setButtonText (U ("Hai unha versión nova: ") + nv + U (" · descargar"));
        newVersion.setVisible (nv.isNotEmpty());
        repaint();
    }
}

void MainView::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161e33), 0.0f, 0.0f, juce::Colour (0xff0b101e), 0.0f, (float) H, false));
    g.fillAll();

    // cabeceira: o panel, a placa co nome e os pozos das teclas
    paintPanel (g, headerArea.toFloat(), 14.0f);
    {
        const auto plate = juce::Rectangle<float> ((float) headerArea.getX() + 10.0f, (float) headerArea.getY() + 6.0f, 188.0f, (float) headerArea.getHeight() - 12.0f);
        paintWell (g, plate, 9.0f);
        g.setColour (col::crema);
        g.setFont (cond (14.5f).withExtraKerningFactor (0.07f));
        g.drawText ("BARULLO XUNTOS", plate.reduced (14.0f, 0.0f).withTrimmedBottom (14.0f).withTrimmedTop (4.0f), juce::Justification::centredLeft, false);
        auto sub = plate.reduced (14.0f, 0.0f).withTop (plate.getBottom() - 17.0f).withHeight (11.0f);
        g.setFont (caps (7.5f, 0.36f));
        const float we = juce::GlyphArrangement::getStringWidth (caps (7.5f, 0.36f), "ESCOITA");
        g.setColour (col::crema2); g.drawText ("ESCOITA", sub, juce::Justification::centredLeft, false);
        g.setColour (col::laranxa); g.drawText (".", sub.withTrimmedLeft (we - 1.5f), juce::Justification::centredLeft, false);
        g.setColour (col::crema2); g.drawText (U (" · FERRAMENTAS"), sub.withTrimmedLeft (we + 4.0f), juce::Justification::centredLeft, false);
    }
    auto well = [&] (juce::Component& a, juce::Component& b) { paintWell (g, a.getBounds().getUnion (b.getBounds()).expanded (4).toFloat(), 11.0f); };
    well (prevComb, nextComb);
    well (presetsBtn, redoBtn);

    // a pantalla: a combinación e o mapa do teclado
    const auto sc = screenArea.toFloat();
    paintGlass (g, sc, 12.0f);
    {
        auto in = sc.reduced (18.0f, 12.0f).withWidth ((float) (zones.getX() - screenArea.getX()) - 36.0f);
        g.setColour (col::crema3);
        drawCaps (g, U ("Combinación ") + juce::String (proc.getCombinacion() + 1), in.removeFromTop (12.0f), juce::Justification::centredLeft, 8.0f, 0.24f);
        in.removeFromTop (2.0f);
        g.setFont (cond (26.0f));
        g.setColour (col::ouro);
        g.drawFittedText (upper (proc.nomeCombinacion()), in.removeFromTop (32.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (col::crema2);
        g.setFont (body (12.5f));
        const auto& cb = xuntos::COMBINACIONS[juce::jlimit (0, xuntos::NUM_COMBINACIONS - 1, proc.getCombinacion())];
        const bool fabrica = proc.nomeCombinacion() == U (cb.nome);
        auto texto = message.isNotEmpty() ? message : fabrica ? U (cb.descricion) : juce::String();
        if (message.isNotEmpty()) g.setColour (col::laranxa);
        g.drawFittedText (texto, in.removeFromTop (34.0f).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
        // a saída
        auto m = in.removeFromBottom (6.0f);
        paintWell (g, m, 3.0f);
        const float lv = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels (proc.nivelSaida.load(), -60.0f) / 60.0f + 1.0f);
        if (lv > 0.0f) { g.setColour (col::laranxa); g.fillRoundedRectangle (m.withWidth (m.getWidth() * lv).reduced (1.0f), 2.0f); }
    }
    g.setColour (col::crema3);
    drawCaps (g, "Mapa", juce::Rectangle<float> ((float) zones.getX(), sc.getY() + 6.0f, 60.0f, 10.0f), juce::Justification::centredLeft, 7.5f, 0.3f);

    // os efectos
    paintPanel (g, fxArea.toFloat(), 14.0f);
    auto titulo = [&] (Knob& a, Knob& b, const juce::String& t)
    { groupTitle (g, juce::Rectangle<float> ((float) a.getX(), (float) fxArea.getY() + 8.0f, (float) (b.getRight() - a.getX()), 20.0f), t); };
    titulo (revSize, revVol, "Reverb");
    titulo (ecoTime, ecoVol, "Eco");
    titulo (volume, volume, U ("Xeral"));

    // o pé
    {
        const auto r = juce::Rectangle<float> (22.0f, (float) H - 22.0f, (float) W - 44.0f, 14.0f);
        g.setColour (col::crema3);
        const float we = juce::GlyphArrangement::getStringWidth (caps (8.0f, 0.3f), "ESCOITA");
        drawCaps (g, "ESCOITA", r, juce::Justification::centredLeft, 8.0f, 0.3f);
        g.setColour (col::laranxa);
        drawCaps (g, ".", r.withTrimmedLeft (we - 2.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
        g.setColour (col::crema3);
        drawCaps (g, U (" Laboratorio creativo · v") + escoita::Actualizacions::currentVersion(), r.withTrimmedLeft (we + 3.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
        if (! newVersion.isVisible())
            drawCaps (g, U ("Os motores de Barullo Xuno, Barullo 7FM e Barullo FM, xuntos"), r, juce::Justification::centred, 8.0f, 0.2f);
        drawCaps (g, U ("42°14′N 8°43′W"), r, juce::Justification::centredRight, 8.0f, 0.3f);
    }
}

void MainView::resized()
{
    const int x0 = 16, w = W - 32;
    headerArea = { x0, 14, w, 52 };
    screenArea = { x0, 76, w, 112 };
    layersArea = { x0, 198, w, 452 };
    fxArea = { x0, 660, w, 110 };

    {
        int x = headerArea.getX() + 210;
        const int y = headerArea.getY() + 10, h = 32;
        combBtn.setBounds (x, y, 280, h); x += 292;
        prevComb.setBounds (x, y, 34, h); x += 38;
        nextComb.setBounds (x, y, 34, h); x += 52;
        presetsBtn.setBounds (x, y, 80, h); x += 84;
        abBtn[0].setBounds (x, y, 38, h); x += 42;
        abBtn[1].setBounds (x, y, 38, h); x += 46;
        undoBtn.setBounds (x, y, 34, h); x += 38;
        redoBtn.setBounds (x, y, 34, h);
        funcBtn.setBounds (headerArea.getRight() - 10 - 96, y, 96, h);
    }
    zones.setBounds (screenArea.getRight() - 18 - 690, screenArea.getY() + 18, 690, screenArea.getHeight() - 28);

    const int gap = 10, sw = (layersArea.getWidth() - 3 * gap) / BXTProcessor::CAPAS;
    for (int k = 0; k < BXTProcessor::CAPAS; ++k) strips[k]->setBounds (layersArea.getX() + k * (sw + gap), layersArea.getY(), sw, layersArea.getHeight());
    editor.setBounds (layersArea);

    {
        const int y = fxArea.getY() + 30, kw = 70, kh = 74;
        int x = fxArea.getX() + 16;
        for (auto* kn : { &revSize, &revBright, &revPre, &revVol }) { kn->setBounds (x, y, kw, kh); x += kw; }
        x += 28;
        ritmoBtn.setBounds (x, y + 6, 118, 28);
        cruzBtn.setBounds (x, y + 40, 118, 28);
        x += 128;
        for (auto* kn : { &ecoTime, &ecoFb, &ecoTone, &ecoVol }) { kn->setBounds (x, y, kw, kh); x += kw; }
        const int rx = fxArea.getRight() - 16 - kw;
        volume.setBounds (rx, y, kw, kh);
        listenBtn.setBounds (rx - 20 - 150, y + 22, 150, 30);
    }
    newVersion.setBounds (W / 2 - 170, H - 27, 340, 22);
}
} // namespace ui

// ================================================================ xanela (escalable)
BXTEditor::BXTEditor (BXTProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 5 / 10, ui::MainView::H * 5 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    float s = proc.uiScale;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto ua = d->userArea;
        const float fit = juce::jmin ((float) (ua.getWidth() - 40) / (float) ui::MainView::W, (float) (ua.getHeight() - 90) / (float) ui::MainView::H);
        if (s > fit && fit > 0.4f) s = fit;
    }
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void BXTEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
