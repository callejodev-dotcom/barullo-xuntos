// MotorXuno.cpp — O Juno-106 de Barullo Xuno nunha capa.
// Son: os 18 bytes do Juno-106 (os do SysEx) e, coma en Barullo Xuno, o que o Juno non garda co
// son pero alí si: portamento aceso (byte 18), o seu tempo (19, 0–255) e a asignación (20).
#include "Motor.h"
#include <escoita_nucleo/escoita_nucleo.h>
#include "barullo-xuno/Source/xuno/Synth.h"
#include "barullo-xuno/Source/xuno/Lecturas.h"
#include "barullo-xuno/Source/Banco.h"
#include "barullo-xuno/Source/Conhecidos.h"

namespace xuntos {
namespace {

using escoita::U;
using escoita::numero;
constexpr int PORTA_ON = 18, PORTA = 19, MODO = 20, BYTES = 21;

xuno::Patch patchDe (const Son& s)
{
    xuno::Patch p {};
    for (int i = 0; i < xuno::PATCH_SIZE; ++i) p[(size_t) i] = s.d[(size_t) i];
    return p;
}

Son sonDe (const xuno::Patch& in, bool portaOn = false, int porta = 90, int modo = 0)
{
    auto p = in; xuno::normalize (p);
    Son s; s.n = BYTES;
    for (int i = 0; i < xuno::PATCH_SIZE; ++i) s.d[(size_t) i] = p[(size_t) i];
    s.d[PORTA_ON] = portaOn ? 1 : 0; s.d[PORTA] = (uint8_t) porta; s.d[MODO] = (uint8_t) modo;
    return s;
}

class MotorXuno : public Motor
{
public:
    void preparar (double fs) override
    {
        s.prepare (fs);
        s.setLfoSens (110);   // os mandos coma ao abrir Barullo Xuno
        rangoBend (2); afinar (0);
        setSon (actual.n == BYTES ? actual : sonDe (xuno::initPatch()));
    }
    void calar() override { s.allSoundOff(); }
    void soltarTodas() override { s.allNotesOff(); }
    void noteOn (int nota, int) override { s.noteOn (nota); }   // o Juno-106 non ten velocidade
    void noteOff (int nota) override { s.noteOff (nota); }
    void pedal (bool abaixo) override { s.sustain (abaixo); }
    void bend (int v) override { s.pitchBend14 (v); }
    void roda (int v) override { s.modWheel (v); }
    void rangoBend (int st) override
    {
        // a sensibilidade do bender (0–255) que dá eses semitóns
        s.setBendSens (juce::jlimit (0, 255, juce::roundToInt (st / xuno::lectura::bendDcoSemitonos (1.0))), 0);
    }
    void afinar (double cents) override { s.setTune (juce::jlimit (0, 255, 0x80 - 8 + juce::roundToInt (cents * 2.56))); }
    void render (float* L, float* R, int n) override { s.render (L, R, n); }
    int vocesActivas() const override { return s.activeVoices(); }
    void setSon (const Son& son) override
    {
        if (son.n != BYTES) return;
        actual = son;
        s.setPatch (patchDe (son));
        s.setPortamento (son.d[PORTA], son.d[PORTA_ON] != 0);
        s.setMode (son.d[MODO]);
    }
    void editar (int byte, uint8_t v) override
    {
        if (byte < 0 || byte >= BYTES) return;
        actual.d[(size_t) byte] = v;
        if (byte < xuno::PATCH_SIZE) s.setPatchByte (byte, v);
        else if (byte == MODO) s.setMode (v);
        else s.setPortamento (actual.d[PORTA], actual.d[PORTA_ON] != 0);
    }

private:
    xuno::Synth s;
    Son actual;
};

std::vector<Banco> bancos()
{
    Banco b;
    b.nome = "XUNO";
    b.titulo = U ("Os 128 sons de Barullo Xuno: A, clásicos; B, Escoita");
    for (int i = 0; i < 128; ++i)
    {
        const auto& e = xuno::bank::PATCHES[i];
        xuno::Patch p {};
        for (int k = 0; k < xuno::PATCH_SIZE; ++k) p[(size_t) k] = e.p[k];
        b.sons.push_back (sonDe (p, e.portaOn, e.porta, e.mode));
        b.nomes.add ((i < 64 ? "A" : "B") + juce::String ((i % 64) / 8 + 1) + juce::String (i % 8 + 1) + " " + U (e.name));
        b.descricions.add (U (e.desc));
    }
    return { b };
}

// o nome: os propios polo son, os de fábrica do Juno-106 pola pegada
juce::String nomeDo (const Son& s)
{
    if (s.n != BYTES) return {};
    const auto p = patchDe (s);
    for (int i = 0; i < 128; ++i)
    {
        xuno::Patch q {};
        for (int k = 0; k < xuno::PATCH_SIZE; ++k) q[(size_t) k] = xuno::bank::PATCHES[i].p[k];
        xuno::normalize (q);
        if (q == p) return U (xuno::bank::PATCHES[i].name);
    }
    const auto h = xuno::patchHash (p);
    for (const auto& f : xuno::known::PATCHES) if (f.hash == h) return U (f.name);
    return {};
}

bool lerSyx (const juce::MemoryBlock& mb, Banco& b, juce::String& erro)
{
    const auto* bytes = static_cast<const uint8_t*> (mb.getData());
    const auto sp = xuno::parseSysex (std::vector<uint8_t> (bytes, bytes + mb.getSize()));
    if (sp.empty()) { erro = U ("Non parece un ficheiro de sons de Juno-106 (.syx con mensaxes de son de Roland)."); return false; }
    std::vector<xuno::Patch> sons;
    if (sp.size() == 1) sons.push_back (sp[0].patch);
    else sons = xuno::sysexToBank (sp);
    if (sons.size() > 128) sons.resize (128);
    const int kb = sons.size() >= 2 ? xuno::known::bankIndex (xuno::bankHash (sons.data(), (int) sons.size())) : -1;
    if (kb >= 0) { b.nome = U (xuno::known::BANKS[kb].name); b.titulo = U (xuno::known::BANKS[kb].title); }
    for (size_t i = 0; i < sons.size(); ++i)
    {
        const auto s = sonDe (sons[i]);
        b.sons.push_back (s);
        const auto nome = nomeDo (s);
        b.nomes.add ((i < 64 ? "A" : "B") + juce::String ((int) (i % 64) / 8 + 1) + juce::String ((int) i % 8 + 1) + (nome.isNotEmpty() ? " " + nome : juce::String()));
        b.descricions.add ({});
    }
    return true;
}

// ---- o editor: o panel do Juno-106, coas lecturas da ROM
juce::String tempo (double ms)
{
    if (ms < 995.0) return juce::String (juce::roundToInt (ms)) + " ms";
    return numero (ms / 1000.0, ms < 9950.0 ? 1 : 0) + " s";
}
juce::String porcento (int v) { return juce::String (juce::roundToInt (v * 100.0 / 127.0)) + " %"; }
juce::String oitavas (double o, bool ambas) { return (ambas ? U ("±") : juce::String ("+")) + numero (o, 1) + " oit"; }

Parametro deslizador (int byte, const char* grupo, const char* nome, const char* axuda, std::function<juce::String (int)> texto, int max = 127)
{
    Parametro p;
    p.nome = U (nome); p.grupo = U (grupo); p.axuda = U (axuda);
    p.min = 0; p.max = max; p.vista = Parametro::DESLIZADOR; p.texto = std::move (texto);
    p.ler = [byte] (const Son& s) { return (int) s.d[(size_t) byte]; };
    p.escribir = [byte] (Son& s, int v) { s.d[(size_t) byte] = (uint8_t) v; };
    return p;
}

Parametro teclas (const char* grupo, const char* nome, const char* axuda, juce::StringArray opcions,
                  std::function<int (const xuno::Patch&)> ler, std::function<void (xuno::Patch&, int)> escribir)
{
    Parametro p;
    p.nome = U (nome); p.grupo = U (grupo); p.axuda = U (axuda);
    p.min = 0; p.max = opcions.size() - 1; p.vista = Parametro::TECLAS; p.opcions = opcions;
    p.ler = [ler] (const Son& s) { return ler (patchDe (s)); };
    p.escribir = [escribir] (Son& s, int v)
    {
        auto q = patchDe (s);
        escribir (q, v);
        for (int i = 0; i < xuno::PATCH_SIZE; ++i) s.d[(size_t) i] = q[(size_t) i];
    };
    return p;
}

std::vector<Parametro> parametros()
{
    namespace lec = xuno::lectura;
    using P = xuno::Patch;
    std::vector<Parametro> v;
    v.push_back (deslizador (xuno::LFO_RATE, "LFO", "Rate", "Velocidade do LFO (triangular).",
        [] (int x) { const double hz = lec::lfoHz (x); return numero (hz, hz < 1.0 ? 2 : 1) + " Hz"; }));
    v.push_back (deslizador (xuno::LFO_DELAY, "LFO", "Delay", "Canto tarda o LFO en chegar despois da primeira nota.",
        [] (int x) { return x == 0 ? juce::String ("0 ms") : tempo (lec::lfoAtrasoMs (x)); }));
    v.push_back (teclas ("DCO", "Rexistro", "O rexistro do DCO.", { "16'", "8'", "4'" },
        [] (const P& p) { return xuno::rangeOf (p); }, [] (P& p, int x) { xuno::setRange (p, x); }));
    v.push_back (deslizador (xuno::DCO_LFO, "DCO", "LFO", "Vibrato: canto move o LFO o ton.",
        [] (int x) { return U ("±") + juce::String (juce::roundToInt (lec::dcoLfoCents (x))) + " c"; }));
    v.push_back (deslizador (xuno::DCO_PWM, "DCO", "PWM", "A anchura do pulso (manual) ou canto a move o LFO.",
        [] (int x) { return juce::String (juce::roundToInt (lec::pwmPorcento (x))) + " %"; }, 105));
    v.push_back (teclas ("DCO", "PWM con", "Quen move a anchura do pulso.", { "LFO", "Man" },
        [] (const P& p) { return p[xuno::SW2] & 1; }, [] (P& p, int x) { p[xuno::SW2] = (uint8_t) ((p[xuno::SW2] & ~1) | (x & 1)); }));
    v.push_back (teclas ("DCO", "Pulso", "A onda de pulso.", { "Off", "On" },
        [] (const P& p) { return (p[xuno::SW1] >> 3) & 1; }, [] (P& p, int x) { p[xuno::SW1] = (uint8_t) ((p[xuno::SW1] & ~8) | (x ? 8 : 0)); }));
    v.push_back (teclas ("DCO", "Serra", "A onda de serra.", { "Off", "On" },
        [] (const P& p) { return (p[xuno::SW1] >> 4) & 1; }, [] (P& p, int x) { p[xuno::SW1] = (uint8_t) ((p[xuno::SW1] & ~16) | (x ? 16 : 0)); }));
    v.push_back (deslizador (xuno::DCO_SUB, "DCO", "Sub", "O sub-oscilador: unha cadrada unha oitava por debaixo.", porcento));
    v.push_back (deslizador (xuno::DCO_NOISE, "DCO", "Ruído", "O ruído.", porcento));
    v.push_back (teclas ("HPF", "Freq", "Filtro paso alto: 0 realza os graves, 1 plano, 2 e 3 cortan.", { "0", "1", "2", "3" },
        [] (const P& p) { return xuno::hpfOf (p); }, [] (P& p, int x) { xuno::setHpf (p, x); }));
    v.push_back (deslizador (xuno::VCF_FREQ, "VCF", "Freq", "O corte do filtro de catro polos.",
        [] (int x) { const double hz = lec::vcfFrecuencia (x); return hz < 1000.0 ? juce::String (juce::roundToInt (hz)) + " Hz" : numero (hz / 1000.0, hz < 10000.0 ? 2 : 1) + " kHz"; }));
    v.push_back (deslizador (xuno::VCF_RES, "VCF", "Res", "Resonancia: preto do máximo o filtro oscila só.", porcento));
    v.push_back (teclas ("VCF", "Env", "A envolvente abre (+) ou pecha (−) o filtro.", { "+", U ("−") },
        [] (const P& p) { return (p[xuno::SW2] >> 1) & 1; }, [] (P& p, int x) { p[xuno::SW2] = (uint8_t) ((p[xuno::SW2] & ~2) | (x ? 2 : 0)); }));
    v.push_back (deslizador (xuno::VCF_ENV, "VCF", "Env", "Canto move a envolvente o corte.", [] (int x) { return oitavas (lec::vcfEnvOitavas (x), false); }));
    v.push_back (deslizador (xuno::VCF_LFO, "VCF", "LFO", "Canto move o LFO o corte.", [] (int x) { return oitavas (lec::vcfLfoOitavas (x), true); }));
    v.push_back (deslizador (xuno::VCF_KYBD, "VCF", "Kybd", "Canto segue o corte ás notas.", porcento));
    v.push_back (teclas ("VCA", "Modo", "O VCA segue a envolvente ou abre e pecha coa tecla.", { "Env", "Gate" },
        [] (const P& p) { return (p[xuno::SW2] >> 2) & 1; }, [] (P& p, int x) { p[xuno::SW2] = (uint8_t) ((p[xuno::SW2] & ~4) | (x ? 4 : 0)); }));
    v.push_back (deslizador (xuno::VCA_LEVEL, "VCA", "Level", "O volume do son: ±10 dB arredor do medio.",
        [] (int x) { const double db = lec::vcaDb (x); return (db >= 0.05 ? "+" : "") + numero (db, 1) + " dB"; }));
    v.push_back (deslizador (xuno::ENV_A, "ENV", "A", "Ataque.", [] (int x) { return tempo (lec::ataqueMs (x)); }));
    v.push_back (deslizador (xuno::ENV_D, "ENV", "D", "Caída ata o sustain (ata −20 dB).", [] (int x) { return tempo (lec::caidaMs (x)); }));
    v.push_back (deslizador (xuno::ENV_S, "ENV", "S", "Nivel de sustain.", porcento));
    v.push_back (deslizador (xuno::ENV_R, "ENV", "R", "Relaxación ao soltar a tecla (ata −20 dB).", [] (int x) { return tempo (lec::caidaMs (x)); }));
    v.push_back (teclas ("Chorus", "Chorus", "Chorus de dous BBD: I (lento) ou II (máis rápido).", { "Off", "I", "II" },
        [] (const P& p) { return xuno::chorusOn (p) ? (xuno::chorusII (p) ? 2 : 1) : 0; },
        [] (P& p, int x) { p[xuno::SW1] = (uint8_t) ((p[xuno::SW1] & ~0x60) | (x == 0 ? 0x60 : x == 1 ? 0x40 : 0x00)); }));

    // o que Barullo Xuno garda co son
    Parametro po;
    po.nome = "Porta"; po.grupo = U ("Execución"); po.axuda = U ("O portamento: as notas escorregan dunha á seguinte.");
    po.max = 1; po.vista = Parametro::TECLAS; po.opcions = { "Off", "On" };
    po.ler = [] (const Son& s) { return (int) s.d[PORTA_ON]; };
    po.escribir = [] (Son& s, int x) { s.d[PORTA_ON] = (uint8_t) x; };
    v.push_back (po);
    Parametro pt;
    pt.nome = "Tempo"; pt.grupo = U ("Execución"); pt.axuda = U ("O tempo do portamento (unha oitava).");
    pt.max = 255; pt.vista = Parametro::DESLIZADOR;
    pt.texto = [] (int x) { return x == 0 ? juce::String ("0 ms") : tempo (lec::portamentoMs (x)); };
    pt.ler = [] (const Son& s) { return (int) s.d[PORTA]; };
    pt.escribir = [] (Son& s, int x) { s.d[PORTA] = (uint8_t) x; };
    v.push_back (pt);
    Parametro pm;
    pm.nome = U ("Asignación"); pm.grupo = U ("Execución"); pm.axuda = U ("POLY 1, POLY 2 (sempre a primeira voz libre) ou as seis voces nunha nota.");
    pm.max = 2; pm.vista = Parametro::TECLAS; pm.opcions = { "Poly 1", "Poly 2", U ("Unís.") };
    pm.ler = [] (const Son& s) { return (int) s.d[MODO]; };
    pm.escribir = [] (Son& s, int x) { s.d[MODO] = (uint8_t) x; };
    v.push_back (pm);
    return v;
}

} // namespace

const Fabrica& fabricaXuno()
{
    static const Fabrica f { "Xuno", "Juno-106: sustractiva", [] () -> std::unique_ptr<Motor> { return std::make_unique<MotorXuno>(); },
                             bancos, nomeDo, lerSyx, parametros, nullptr, 1.0 };
    return f;
}

} // namespace xuntos
