// MotorFM.cpp — O chip FM de 2 operadores do teclado de Barullo FM nunha capa, coa súa cadea de
// saída (o ecualizador do altofalante do teclado, coma ao abrir o plugin).
// Son: o número do son (0–99) e os rexistros das súas dúas partes (r[0..15] e o de canle, g7).
#include "Motor.h"
#include <escoita_nucleo/escoita_nucleo.h>
#include "barullofm/Source/Synth.h"
#include "barullofm/Source/Fx.h"

namespace xuntos {
namespace {

using escoita::U;
constexpr int BYTES = 1 + 2 * 17;

int base (int parte) { return 1 + parte * 17; }   // r[k] en base + k, g7 en base + 16

Son sonDe (int voz)
{
    Son s; s.n = BYTES;
    s.d[0] = (uint8_t) voz;
    for (int pa = 0; pa < 2; ++pa)
    {
        const auto& p = ym::VOICES[voz].parts[pa];
        for (int k = 0; k < 16; ++k) s.d[(size_t) (base (pa) + k)] = p.r[k];
        s.d[(size_t) (base (pa) + 16)] = p.g7;
    }
    return s;
}

class MotorFM : public Motor
{
public:
    void preparar (double fs) override
    {
        s.prepare (fs);
        fx.eqOn = true; fx.speakerOn = false; fx.volumeDb = 0;   // coma ao abrir Barullo FM
        fx.prepare (fs);
        s.setFilms (true);
        s.setUnlimited (false);
        s.setVelocitySens (0.0);
        rango = 2; afinado = 0; aplicarBend();
        setSon (actual.n == BYTES ? actual : sonDe (0));
    }
    void calar() override { s.reset(); fx.reset(); }
    void soltarTodas() override { s.allNotesOff(); }
    void noteOn (int nota, int vel) override { s.midiNoteOn (nota, vel); }
    void noteOff (int nota) override { s.midiNoteOff (nota); }
    void pedal (bool abaixo) override { s.midiSustain (abaixo); }
    void bend (int v) override { roda14 = v; aplicarBend(); }
    void roda (int v) override { s.midiModWheel (v / 127.0); }
    void rangoBend (int st) override { if (st != rango) { rango = st; aplicarBend(); } }
    void afinar (double c) override { if (std::abs (c - afinado) > 1.0e-9) { afinado = c; aplicarBend(); } }
    void render (float* L, float* R, int n) override
    {
        std::fill (L, L + n, 0.0f); std::fill (R, R + n, 0.0f);
        s.render (L, R, n);
        fx.process (L, R, n);
    }
    int vocesActivas() const override { return s.activeTotal(); }
    void setSon (const Son& son) override
    {
        if (son.n != BYTES) return;
        actual = son;
        const int voz = std::min<int> (son.d[0], ym::NUM_VOICES - 1);
        s.setVoice (voz);
        s.resetPatch (voz);
        for (int pa = 0; pa < 2; ++pa)
        {
            for (int k = 2; k <= 13; ++k) s.setPatchReg (voz, pa, k, son.d[(size_t) (base (pa) + k)]);
            s.setPatchReg (voz, pa, 16, son.d[(size_t) (base (pa) + 16)]);
        }
    }
    void editar (int byte, uint8_t v) override
    {
        if (byte < 0 || byte >= BYTES) return;
        actual.d[(size_t) byte] = v;
        if (byte == 0) { setSon (actual); return; }
        const int pa = (byte - 1) / 17, k = (byte - 1) % 17;
        if (k == 16 || (k >= 2 && k <= 13)) s.setPatchReg (actual.d[0], pa, k, v);
    }

private:
    // o bend e a afinación fina xuntos: o motor fai bend × alcance en semitóns, en dobre precisión
    void aplicarBend()
    {
        const double r = std::max (1, rango) + (std::abs (afinado) > 1.0e-9 ? 1 : 0);
        const double st = (roda14 - 8192) / 8192.0 * rango + afinado / 100.0;
        s.setBendRange (r);
        s.midiPitchBend (st / r);
    }

    ym::Synth s;
    ym::OutputChain fx;
    Son actual;
    int rango = 2, roda14 = 8192;
    double afinado = 0;
};

std::vector<Banco> bancos()
{
    Banco b;
    b.nome = "FM";
    b.titulo = U ("Os 100 sons do teclado, cos seus movementos");
    for (int v = 0; v < ym::NUM_VOICES; ++v)
    {
        b.sons.push_back (sonDe (v));
        b.nomes.add (juce::String (v).paddedLeft ('0', 2) + " " + U (ym::VOICES[v].name));
        b.descricions.add (U (ym::VOICES[v].desc));
    }
    return { b };
}

juce::String nomeDo (const Son& s)
{
    return s.n == BYTES ? U (ym::VOICES[std::min<int> (s.d[0], ym::NUM_VOICES - 1)].name) : juce::String();
}

// ---- o editor: modulador e portadora de cada parte, coma no editor de Barullo FM
Parametro campo (int parte, int k, int shift, int mascara, const juce::String& grupo, const juce::String& nome, const juce::String& axuda,
                 Parametro::Vista vista = Parametro::DESLIZADOR, std::function<juce::String (int)> texto = {})
{
    Parametro p;
    p.nome = nome; p.grupo = grupo; p.axuda = axuda;
    p.min = 0; p.max = mascara; p.vista = vista; p.texto = std::move (texto);
    if (vista == Parametro::TECLAS) p.opcions = { "Off", "On" };
    const int byte = base (parte) + k;
    p.ler = [byte, shift, mascara] (const Son& s) { return (s.d[(size_t) byte] >> shift) & mascara; };
    p.escribir = [byte, shift, mascara] (Son& s, int v)
    {
        auto& b = s.d[(size_t) byte];
        b = (uint8_t) ((b & ~(mascara << shift)) | ((v & mascara) << shift));
    };
    return p;
}

std::vector<Parametro> parametros()
{
    static const float MULT[16] = { 0.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15 };
    std::vector<Parametro> v;
    for (int pa = 0; pa < 2; ++pa)
    {
        const auto parte = pa == 0 ? U ("Parte 1") : U ("Parte 2 (sons de dúas canles)");
        for (int op = 0; op < 2; ++op)
        {
            const auto g = parte + U (" · ") + (op == 0 ? U ("Modulador") : U ("Portadora"));
            v.push_back (campo (pa, 2 + op, 0, 15, g, "Mult", U ("Cantas veces a frecuencia da nota."), Parametro::DESLIZADOR,
                [] (int x) { return U ("×") + juce::String (MULT[x & 15], x == 0 ? 1 : 0); }));
            v.push_back (campo (pa, 4 + op, 0, 255, g, "Nivel", U ("Atenuación, en pasos de 0,1875 dB."), Parametro::DESLIZADOR,
                [] (int x) { return x == 0 ? juce::String ("0 dB") : U ("−") + juce::String (x * 0.1875, 1) + " dB"; }));
            v.push_back (campo (pa, 6 + op, 4, 15, g, "A", U ("Ataque (15 = instantáneo).")));
            v.push_back (campo (pa, 6 + op, 0, 15, g, "D", U ("Caída ata o sostemento.")));
            v.push_back (campo (pa, 8 + op, 4, 15, g, "S", U ("Nivel de sostemento."), Parametro::DESLIZADOR,
                [] (int x) { return x == 0 ? juce::String ("0 dB") : x == 15 ? U ("−93 dB") : U ("−") + juce::String (x * 3) + " dB"; }));
            v.push_back (campo (pa, 8 + op, 0, 15, g, "R", U ("Relaxación.")));
            v.push_back (campo (pa, 2 + op, 4, 1, g, "KSR", U ("A envolvente vai máis rápida nas notas agudas."), Parametro::TECLAS));
            v.push_back (campo (pa, 2 + op, 5, 1, g, "1/2", U ("Media onda: só a metade positiva do seno."), Parametro::TECLAS));
        }
        v.push_back (campo (pa, 16, 0, 7, parte, "FB", U ("Realimentación do modulador."), Parametro::DESLIZADOR));
        v.push_back (campo (pa, 16, 7, 1, parte, "Mesma", U ("O modulador usa a frecuencia da portadora."), Parametro::TECLAS));
        v.push_back (campo (pa, 16, 6, 1, parte, "Esq.", U ("A canle sae polo lado esquerdo."), Parametro::TECLAS));
        v.push_back (campo (pa, 16, 5, 1, parte, "Der.", U ("A canle sae polo lado dereito."), Parametro::TECLAS));
    }
    return v;
}

} // namespace

const Fabrica& fabricaFM()
{
    static const Fabrica f { "FM", "Chip FM de 2 operadores dun teclado", [] () -> std::unique_ptr<Motor> { return std::make_unique<MotorFM>(); },
                             bancos, nomeDo, nullptr, parametros, nullptr, 0.9 };   // −0,9 dB: medido cos acordes dos sons (BXTRenderTest --niveis)
    return f;
}

} // namespace xuntos
