// Editores.cpp — O editor do son dunha capa. Monta os controis cos parámetros do motor
// (xuntos::Parametro): o Juno como o seu panel (deslizadores e teclas por seccións), o DX7 e o
// chip FM como unha táboa (unha fila por operador ou parte). Cada cambio vai ao motor ao
// momento e pódese desfacer.
#include "PluginEditor.h"

namespace
{
    using escoita::U;
    bool interruptor (const xuntos::Parametro& p) { return p.vista == xuntos::Parametro::TECLAS && p.opcions.size() == 2 && p.opcions[0] == "Off"; }
}

namespace ui
{
SoundEditor::SoundEditor (BXTProcessor& p, MainView& m) : proc (p), main (m)
{
    restoreBtn.getProperties().set ("noLed", true);
    closeBtn.getProperties().set ("noLed", true);
    restoreBtn.onClick = [this] { proc.restaurarSon (k, tipo); refresh(); };
    closeBtn.onClick = [this] { main.closeEditor(); };
    restoreBtn.setTooltip (U ("Volver ao son tal como está no banco."));
    for (auto* b : { &restoreBtn, &closeBtn }) { b->setWantsKeyboardFocus (false); addAndMakeVisible (b); }
}

void SoundEditor::open (int layer)
{
    k = layer;
    build();
}

void SoundEditor::build()
{
    tipo = proc.tipoCapa (k);
    panelMode = tipo == xuntos::XUNO;
    controls.clear();
    groups.clear();
    const auto cor = (juce::int64) col::capa (k).getARGB();
    for (auto& p : xuntos::fabrica (tipo).parametros())
    {
        groups.addIfNotAlreadyThere (p.grupo);
        auto c = std::make_unique<Control>();
        c->p = p;
        auto* ctl = c.get();
        if (p.vista == xuntos::Parametro::TECLAS && (panelMode || interruptor (p)))
        {
            // teclas: un interruptor (unha tecla co nome) ou unha opción por tecla
            const int n = interruptor (p) ? 1 : p.opcions.size();
            for (int i = 0; i < n; ++i)
            {
                auto* b = c->keys.add (new juce::TextButton (interruptor (p) ? p.nome : p.opcions[i]));
                b->getProperties().set ("led", cor);
                b->getProperties().set ("fontSize", 8.5f);
                b->setTooltip (p.nome + ": " + p.axuda);
                b->setWantsKeyboardFocus (false);
                b->onClick = [this, ctl, i, n] { set (*ctl, n == 1 ? (ctl->keys[0]->getToggleState() ? 0 : 1) : i); };
                addAndMakeVisible (b);
            }
        }
        else if (panelMode && p.vista == xuntos::Parametro::DESLIZADOR)
        {
            c->fader = std::make_unique<Fader>();
            c->fader->name = p.nome;
            c->fader->slider.setRange (p.min, p.max, 1.0);
            c->fader->slider.setTooltip (p.axuda);
            const auto texto = p.texto;
            c->fader->slider.textFromValueFunction = [texto] (double v) { return texto ? texto ((int) v) : juce::String ((int) v); };
            c->fader->slider.onValueChange = [this, ctl] { set (*ctl, (int) ctl->fader->slider.getValue()); ctl->fader->repaint(); };
            addAndMakeVisible (*c->fader);
        }
        else
        {
            c->num = std::make_unique<NumBox>();
            c->num->name = p.nome;
            c->num->min = p.min; c->num->max = p.max;
            c->num->tint = col::capa (k);
            c->num->setTooltip (p.axuda);
            if (p.vista == xuntos::Parametro::TECLAS) { const auto op = p.opcions; c->num->text = [op] (int v) { return op[v]; }; }
            else if (p.texto) c->num->text = p.texto;
            c->num->onChange = [this, ctl] (int v) { set (*ctl, v); };
            addAndMakeVisible (*c->num);
        }
        controls.push_back (std::move (c));
    }
    resized();
    refresh();
}

void SoundEditor::set (Control& c, int value)
{
    auto s = proc.son (k, tipo);
    if (c.p.ler (s) == value) { refresh(); return; }
    c.p.escribir (s, value);
    proc.editarSon (k, tipo, s);
    refresh();
}

void SoundEditor::refresh()
{
    if (tipo != proc.tipoCapa (k)) { build(); return; }
    const auto& s = proc.son (k, tipo);
    for (auto& c : controls)
    {
        const int v = c->p.ler (s);
        if (c->fader != nullptr) { if ((int) c->fader->slider.getValue() != v) c->fader->slider.setValue (v, juce::dontSendNotification); c->fader->repaint(); }
        if (c->num != nullptr) c->num->setValue (v, false);
        for (int i = 0; i < c->keys.size(); ++i)
            c->keys[i]->setToggleState (c->keys.size() == 1 ? v != 0 : i == v, juce::dontSendNotification);
    }
    restoreBtn.setEnabled (proc.editado (k, tipo));
    repaint();
}

void SoundEditor::resized()
{
    auto r = getLocalBounds().reduced (14, 0);
    closeBtn.setBounds (r.getRight() - 96, 12, 96, 28);
    restoreBtn.setBounds (r.getRight() - 96 - 8 - 110, 12, 110, 28);
    r.removeFromTop (52);
    groupAreas.clear();
    drawing = {};

    if (panelMode)
    {
        // o panel: as seccións en columnas; os deslizadores un a un e as teclas seguidas nunha columna
        const int fw = 40, kw = 56, pad = 10, h = r.getHeight() - 12;
        int x = r.getX();
        for (const auto& g : groups)
        {
            const int x0 = x;
            x += pad / 2;
            juce::Rectangle<int> keyCol;
            int ky = 0;
            for (auto& c : controls)
            {
                if (c->p.grupo != g) continue;
                if (c->fader != nullptr || c->num != nullptr)
                {
                    keyCol = {};
                    juce::Component* comp = c->fader != nullptr ? (juce::Component*) c->fader.get() : (juce::Component*) c->num.get();
                    if (c->fader != nullptr) comp->setBounds (x, r.getY() + 26, fw, h - 26);
                    else comp->setBounds (x, r.getY() + 26 + (h - 26) / 2 - 17, fw + 8, 34);
                    x += c->fader != nullptr ? fw : fw + 8;
                    continue;
                }
                if (keyCol.isEmpty()) { keyCol = { x, r.getY() + 34, kw, h - 34 }; ky = keyCol.getY(); x += kw; }
                const bool solo = c->keys.size() == 1;
                if (! solo) { c->area = { keyCol.getX(), ky, kw, 12 }; ky += 14; }
                for (auto* b : c->keys) { b->setBounds (keyCol.getX() + 3, ky, kw - 6, 26); ky += 30; }
                ky += 6;
            }
            x += pad / 2;
            groupAreas.push_back ({ x0, r.getY(), x - x0, h });
        }
        // se sobra sitio, todo un pouco máis ancho non: céntrase
        const int sobra = r.getRight() - x;
        if (sobra > 0)
        {
            for (auto& c : controls)
            {
                if (c->fader != nullptr) c->fader->setTopLeftPosition (c->fader->getX() + sobra / 2, c->fader->getY());
                if (c->num != nullptr) c->num->setTopLeftPosition (c->num->getX() + sobra / 2, c->num->getY());
                for (auto* b : c->keys) b->setTopLeftPosition (b->getX() + sobra / 2, b->getY());
                c->area.translate (sobra / 2, 0);
            }
            for (auto& a : groupAreas) a.translate (sobra / 2, 0);
        }
        return;
    }

    // a táboa: unha fila por grupo (operador, parte, global), co rótulo á esquerda
    const int lw = labelW(), cw = 40, gap = 2, rowH = 40;
    if (xuntos::fabrica (tipo).pintar != nullptr) drawing = r.removeFromRight (190).reduced (8, 0);
    int y = r.getY();
    for (const auto& g : groups)
    {
        int x = r.getX() + lw;
        for (auto& c : controls)
        {
            if (c->p.grupo != g) continue;
            if (c->num != nullptr) c->num->setBounds (x, y, cw, 34);
            for (auto* b : c->keys) b->setBounds (x, y + 10, cw + 10, 24);
            x += (c->keys.isEmpty() ? cw : cw + 10) + gap;
        }
        groupAreas.push_back ({ r.getX(), y, x - r.getX(), rowH - 4 });
        y += rowH;
    }
}

void SoundEditor::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    paintPanel (g, r, 14.0f);
    const auto cor = col::capa (k);
    g.setColour (cor.withAlpha (0.9f));
    g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 3.0f, r.getWidth() - 28.0f, 2.0f, 1.0f);
    {
        auto t = juce::Rectangle<float> (r.getX() + 18.0f, 12.0f, r.getWidth() - 300.0f, 28.0f);
        g.setColour (cor);
        drawCaps (g, U ("Editar · capa ") + juce::String (k + 1) + U (" · ") + MainView::origin (proc, k, tipo),
                  t.removeFromTop (12.0f), juce::Justification::centredLeft, 8.0f, 0.24f);
        g.setColour (col::crema);
        g.setFont (semi (15.0f));
        g.drawText (upper (proc.nomeSon (k, tipo)) + (proc.editado (k, tipo) ? U ("  ·  EDITADO") : juce::String()), t, juce::Justification::centredLeft, false);
    }
    if (panelMode)
    {
        for (size_t i = 0; i < groupAreas.size(); ++i)
        {
            const auto a = groupAreas[i].toFloat();
            groupTitle (g, a.withHeight (20.0f), groups[(int) i]);
            if (i > 0) { g.setColour (col::lina.withAlpha (0.7f)); g.fillRect (a.getX() - 1.0f, a.getY() + 30.0f, 1.0f, a.getHeight() - 40.0f); }
        }
        g.setColour (col::crema3);
        for (auto& c : controls)
            if (! c->area.isEmpty()) drawCaps (g, c->p.nome, c->area.toFloat(), juce::Justification::centred, 7.0f, 0.2f);
    }
    else
    {
        for (size_t i = 0; i < groupAreas.size(); ++i)
        {
            const auto a = groupAreas[i].toFloat();
            const float lw = (float) labelW() - 6.0f;
            paintWell (g, a.withWidth (lw).withTrimmedTop (10.0f), 6.0f);
            g.setColour (cor);
            g.setFont (caps (8.0f, 0.12f));
            g.drawFittedText (upper (groups[(int) i]), a.withWidth (lw).withTrimmedTop (10.0f).reduced (8.0f, 0.0f).toNearestInt(),
                              juce::Justification::centredLeft, 2, 0.8f);
        }
        if (! drawing.isEmpty())
        {
            paintGlass (g, drawing.toFloat(), 10.0f);
            g.setColour (col::crema3);
            drawCaps (g, "Algoritmo", drawing.toFloat().reduced (10.0f, 8.0f).removeFromTop (10.0f), juce::Justification::centredLeft, 7.5f, 0.24f);
            xuntos::fabrica (tipo).pintar (g, drawing.toFloat().reduced (10.0f, 22.0f), proc.son (k, tipo), col::crema, cor);
        }
    }
}
} // namespace ui
