// Motor7FM.cpp — O DX7 de Barullo 7FM nunha capa.
// Son: os 155 parámetros do DX7 (VCED, primeiro o OP6) e o byte 156, os operadores acesos.
#include "Motor.h"
#include <escoita_nucleo/escoita_nucleo.h>
#include "barullo7fm/Source/Synth7.h"
#include "barullo7fm/Source/Cartuchos.h"
#include "barullo7fm/Source/Banco.h"
#include "barullo7fm/Source/BancoClasicos.h"
#include "barullo7fm/Source/Algo.h"

namespace xuntos {
namespace {

using escoita::U;
constexpr int BYTES = 156, OPS = 155;

b7::Patch patchDe (const Son& s)
{
    b7::Patch p {};
    for (int i = 0; i < BYTES; ++i) p[(size_t) i] = s.d[(size_t) i];
    return p;
}

Son sonDe (b7::Patch p)
{
    const uint8_t mascara = p[OPS] & 0x3f;
    b7::sanitize (p);
    p[OPS] = mascara ? mascara : 0x3f;
    Son s; s.n = BYTES;
    for (int i = 0; i < BYTES; ++i) s.d[(size_t) i] = p[(size_t) i];
    return s;
}

class Motor7FM : public Motor
{
public:
    void preparar (double fs) override
    {
        s.prepare (fs);
        s.setWheel (99, true, false, false);   // os mandos coma ao abrir Barullo 7FM
        rango = 2; afinado = 0; aplicarBend();
        setSon (actual.n == BYTES ? actual : sonDe (b7::initPatch()));
    }
    void calar() override { s.allSoundOff(); }
    void soltarTodas() override { s.allNotesOff(); }
    void noteOn (int nota, int vel) override { s.noteOn (nota, vel); }
    void noteOff (int nota) override { s.noteOff (nota); }
    void pedal (bool abaixo) override { s.setSustain (abaixo); }
    void bend (int v) override { roda14 = v; aplicarBend(); }
    void roda (int v) override { s.modWheel (v); }
    void rangoBend (int st) override { if (st != rango) { rango = st; aplicarBend(); } }
    void afinar (double c) override { if (std::abs (c - afinado) > 1.0e-9) { afinado = c; aplicarBend(); } }
    void render (float* L, float* R, int n) override
    {
        s.render (L, n);   // o DX7 é mono
        std::copy (L, L + n, R);
    }
    int vocesActivas() const override { return s.activeVoices(); }
    void setSon (const Son& son) override
    {
        if (son.n != BYTES) return;
        actual = son;
        const auto p = patchDe (son);
        s.setPatch (p);
        s.setOpSwitch (p[OPS]);
    }
    void editar (int byte, uint8_t v) override
    {
        if (byte < 0 || byte >= BYTES) return;
        actual.d[(size_t) byte] = v;
        if (byte == OPS) s.setOpSwitch (v);
        else s.editPatch (byte, v);
    }

private:
    // a afinación fina vai polo bend do DX7 (o seu master tune non se toca desde fóra): o alcance
    // sobe un semitón para que caiba e o valor leva a roda e os cents xuntos
    void aplicarBend()
    {
        if (std::abs (afinado) < 1.0e-9) { s.setBendRange (rango); s.pitchBend (roda14); return; }
        const int r = std::min (12, rango + 1);
        const double st = (roda14 - 8192) / 8192.0 * rango + afinado / 100.0;
        s.setBendRange (r);
        s.pitchBend (juce::jlimit (0, 16383, 8192 + juce::roundToInt (st / r * 8192.0)));
    }

    b7::Synth7 s;
    Son actual;
    int rango = 2, roda14 = 8192;
    double afinado = 0;
};

std::vector<Banco> bancos()
{
    std::vector<Banco> v;
    for (int c = 0; c < b7::NUM_CARTRIDGES; ++c)
    {
        const auto& cart = b7::CARTRIDGES[c];
        Banco b;
        b.nome = cart.name;
        b.titulo = U (cart.title);
        for (int k = 0; k < 32; ++k)
        {
            const auto p = b7::unpackVmem (cart.vmem + k * 128);
            b.sons.push_back (sonDe (p));
            b.nomes.add (juce::String (k + 1).paddedLeft ('0', 2) + " " + juce::String (b7::patchName (p)));
            b.descricions.add (U (cart.descKind == 1 ? b7::CLASICOS_DESC[k] : cart.descKind == 2 ? b7::FACTORY[k].desc : ""));
        }
        v.push_back (b);
    }
    return v;
}

juce::String nomeDo (const Son& s) { return s.n == BYTES ? juce::String (b7::patchName (patchDe (s))) : juce::String(); }

bool lerSyx (const juce::MemoryBlock& mb, Banco& b, juce::String& erro)
{
    const auto* bytes = static_cast<const uint8_t*> (mb.getData());
    const auto sons = b7::parseSysex (std::vector<uint8_t> (bytes, bytes + mb.getSize()));
    if (sons.empty()) { erro = U ("Non parece un ficheiro de sons de DX7 (.syx dun son ou dun banco de 32)."); return false; }
    if (sons.size() >= 32)
    {
        const auto h = b7::bankHash (sons.data());
        for (int i = 0; i < b7::NUM_KNOWN_BANKS; ++i)
            if (b7::KNOWN_BANKS[i].hash == h) { b.nome = b7::KNOWN_BANKS[i].name; b.titulo = U (b7::KNOWN_BANKS[i].title); }
    }
    for (size_t k = 0; k < sons.size() && k < 32; ++k)
    {
        b.sons.push_back (sonDe (sons[k]));
        b.nomes.add (juce::String ((int) k + 1).paddedLeft ('0', 2) + " " + juce::String (b7::patchName (sons[k])));
        b.descricions.add ({});
    }
    return true;
}

// ---- o editor: os seis operadores e o global, coma no DX7
Parametro numero (int byte, const juce::String& grupo, const juce::String& nome, int max, const juce::String& axuda = {},
                  std::function<juce::String (int)> texto = {})
{
    Parametro p;
    p.nome = nome; p.grupo = grupo; p.axuda = axuda;
    p.min = 0; p.max = max; p.vista = Parametro::NUMERO; p.texto = std::move (texto);
    p.ler = [byte] (const Son& s) { return (int) s.d[(size_t) byte]; };
    p.escribir = [byte] (Son& s, int v) { s.d[(size_t) byte] = (uint8_t) v; };
    return p;
}

std::vector<Parametro> parametros()
{
    std::vector<Parametro> v;
    auto curva = [] (int x) { static const char* c[4] = { "-LIN", "-EXP", "+EXP", "+LIN" }; return juce::String (c[x & 3]).replaceCharacter ('-', (juce::juce_wchar) 0x2212); };
    for (int op = 1; op <= 6; ++op)
    {
        const int b = (6 - op) * 21;
        const auto g = "OP" + juce::String (op);
        // aceso ou apagado (o byte 156: bit 0 = OP6 … bit 5 = OP1)
        Parametro on;
        on.nome = "On"; on.grupo = g; on.axuda = U ("O operador soa ou cala (coma as teclas de operador do DX7).");
        on.max = 1; on.vista = Parametro::TECLAS; on.opcions = { "Off", "On" };
        const int bit = 6 - op;
        on.ler = [bit] (const Son& s) { return (s.d[OPS] >> bit) & 1; };
        on.escribir = [bit] (Son& s, int x) { s.d[OPS] = (uint8_t) ((s.d[OPS] & ~(1 << bit)) | ((x & 1) << bit)); };
        v.push_back (on);
        for (int i = 0; i < 4; ++i) v.push_back (numero (b + i, g, "R" + juce::String (i + 1), 99, U ("Velocidade do tramo ") + juce::String (i + 1) + U (" da envolvente.")));
        for (int i = 0; i < 4; ++i) v.push_back (numero (b + 4 + i, g, "L" + juce::String (i + 1), 99, U ("Nivel ao final do tramo ") + juce::String (i + 1) + "."));
        v.push_back (numero (b + 8, g, "BP", 99, U ("Punto de ruptura do escalado de nivel polo teclado."),
            [] (int x) { static const char* n[12] = { "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" }; return juce::String (n[x % 12]) + juce::String ((x + 9) / 12 - 1); }));
        v.push_back (numero (b + 9, g, "LD", 99, U ("Canto cambia o nivel cara aos graves.")));
        v.push_back (numero (b + 10, g, "RD", 99, U ("Canto cambia o nivel cara aos agudos.")));
        v.push_back (numero (b + 11, g, "LC", 3, U ("Curva cara aos graves."), curva));
        v.push_back (numero (b + 12, g, "RC", 3, U ("Curva cara aos agudos."), curva));
        v.push_back (numero (b + 13, g, "RS", 7, U ("Escalado do tempo: as notas agudas van máis rápidas.")));
        v.push_back (numero (b + 14, g, "AMS", 3, U ("Canto lle afecta o LFO á amplitude.")));
        v.push_back (numero (b + 15, g, "KVS", 7, U ("Sensibilidade á velocidade.")));
        v.push_back (numero (b + 16, g, "OL", 99, U ("Nivel de saída do operador.")));
        v.push_back (numero (b + 17, g, "M", 1, U ("Frecuencia proporcional á nota (R) ou fixa (F)."), [] (int x) { return juce::String (x ? "F" : "R"); }));
        v.push_back (numero (b + 18, g, "FC", 31, U ("Frecuencia grosa (no modo proporcional, ×0,5 a ×31).")));
        v.push_back (numero (b + 19, g, "FF", 99, U ("Frecuencia fina.")));
        v.push_back (numero (b + 20, g, "DET", 14, U ("Desafinación (−7 a +7)."), [] (int x) { return (x > 7 ? "+" : "") + escoita::numero (x - 7, 0); }));
    }
    const auto g = U ("Global");
    for (int i = 0; i < 4; ++i) v.push_back (numero (126 + i, g, "PR" + juce::String (i + 1), 99, U ("Velocidade do tramo da envolvente de ton.")));
    for (int i = 0; i < 4; ++i) v.push_back (numero (130 + i, g, "PL" + juce::String (i + 1), 99, U ("Nivel da envolvente de ton (50 = sen cambio).")));
    v.push_back (numero (134, g, "ALG", 31, U ("Algoritmo: como se conectan os seis operadores."), [] (int x) { return juce::String (x + 1); }));
    v.push_back (numero (135, g, "FB", 7, U ("Realimentación.")));
    v.push_back (numero (136, g, "OKS", 1, U ("Os osciladores empezan sempre na mesma fase.")));
    v.push_back (numero (137, g, "LFS", 99, U ("Velocidade do LFO.")));
    v.push_back (numero (138, g, "LFD", 99, U ("Atraso do LFO.")));
    v.push_back (numero (139, g, "LPMD", 99, U ("Canto move o LFO o ton.")));
    v.push_back (numero (140, g, "LAMD", 99, U ("Canto move o LFO a amplitude.")));
    v.push_back (numero (141, g, "LKS", 1, U ("O LFO empeza de novo con cada nota.")));
    v.push_back (numero (142, g, "LFW", 5, U ("Onda do LFO."), [] (int x) { static const char* n[6] = { "TRI", "SAW-", "SAW+", "SQR", "SIN", "S/H" }; return juce::String (n[juce::jlimit (0, 5, x)]); }));
    v.push_back (numero (143, g, "LPMS", 7, U ("Sensibilidade do ton ao LFO.")));
    v.push_back (numero (144, g, "TRNSP", 48, U ("Transposición (C3 = sen transpor)."),
        [] (int x) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return juce::String (n[x % 12]) + juce::String (x / 12 + 1); }));
    return v;
}

void pintar (juce::Graphics& g, juce::Rectangle<float> r, const Son& s, juce::Colour tinta, juce::Colour acento)
{
    if (s.n != BYTES) return;
    const auto G = b7::algoGraph (s.d[134]);
    float maxX = 0, maxY = 0;
    for (int o = 1; o <= 6; ++o) { maxX = std::max (maxX, G.x[(size_t) o]); maxY = std::max (maxY, G.y[(size_t) o]); }
    const float cw = std::min (46.0f, r.getWidth() / (maxX + 1.6f)), rh = std::min (40.0f, (r.getHeight() - 16.0f) / (maxY + 1.4f));
    const float bw = cw * 0.66f, bh = rh * 0.62f;
    const float x0 = r.getCentreX() - maxX * cw / 2, y0 = r.getCentreY() + maxY * rh / 2;   // centrado
    auto pos = [&] (int o) { return juce::Point<float> (x0 + G.x[(size_t) o] * cw, y0 - G.y[(size_t) o] * rh); };
    g.setColour (tinta.withAlpha (0.6f));
    for (auto [m, d] : G.edges)
    {
        const auto a = pos (m), b = pos (d);
        juce::Path p; p.startNewSubPath (a.x, a.y + bh / 2); p.lineTo (a.x, (a.y + b.y) / 2); p.lineTo (b.x, (a.y + b.y) / 2); p.lineTo (b.x, b.y - bh / 2);
        g.strokePath (p, juce::PathStrokeType (1.3f));
    }
    {   // a realimentación
        const auto a = pos (G.fbFrom), b = pos (G.fbTo);
        const float rx = std::max (a.x, b.x) + bw / 2 + 6;
        juce::Path p; p.startNewSubPath (a.x, a.y + bh / 2); p.lineTo (a.x, a.y + bh / 2 + 5); p.lineTo (rx, a.y + bh / 2 + 5);
        p.lineTo (rx, b.y - bh / 2 - 5); p.lineTo (b.x, b.y - bh / 2 - 5); p.lineTo (b.x, b.y - bh / 2);
        g.setColour (acento.withAlpha (0.8f));
        g.strokePath (p, juce::PathStrokeType (1.3f));
    }
    for (int o = 1; o <= 6; ++o)
    {
        const auto c = pos (o);
        const auto box = juce::Rectangle<float> (bw, bh).withCentre (c);
        const bool on = (s.d[OPS] >> (6 - o)) & 1;
        g.setColour (G.carrier[(size_t) o] ? acento.withAlpha (on ? 0.85f : 0.25f) : tinta.withAlpha (on ? 0.25f : 0.08f));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (tinta.withAlpha (on ? 1.0f : 0.4f));
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
        g.setFont (juce::FontOptions (bh * 0.62f));
        g.drawText (juce::String (o), box, juce::Justification::centred);
    }
}

} // namespace

const Fabrica& fabrica7FM()
{
    static const Fabrica f { "7FM", "DX7: FM de seis operadores", [] () -> std::unique_ptr<Motor> { return std::make_unique<Motor7FM>(); },
                             bancos, nomeDo, lerSyx, parametros, pintar, 1.78 };   // +5 dB: medido cos acordes dos sons (BXTRenderTest --niveis)
    return f;
}

} // namespace xuntos
