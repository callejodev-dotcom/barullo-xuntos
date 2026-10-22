#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Combinacions.h"

using escoita::U;

namespace
{
    // o eco á par do DAW: cantas negras dura cada figura
    constexpr double RITMOS[9] = { 0, 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
    // marxe da mestura: cada motor vén co volume do seu plugin, e as capas súmanse (dúas iguais xa
    // son +6 dB). Con −6 dB, un acorde forte de sete notas en calquera combinación queda por debaixo
    // de −6 dBFS: o limitador só traballa cando se lle pide moito (dez dedos, forte e co pedal).
    constexpr double MESTURA = 0.5;
}

BXTProcessor::BXTProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloXuntos", createLayout())
{
    for (int k = 0; k < CAPAS; ++k)
    {
        auto& c = capas[(size_t) k];
        auto p = [this, k] (const char* n) { return apvts.getRawParameterValue (id (k, n)); };
        c.on = p ("on"); c.motor = p ("motor"); c.volume = p ("volume"); c.pan = p ("pan"); c.oitava = p ("oitava");
        c.semitons = p ("semitons"); c.cents = p ("cents"); c.notaBaixa = p ("notaBaixa"); c.notaAlta = p ("notaAlta");
        c.velBaixa = p ("velBaixa"); c.velAlta = p ("velAlta"); c.canle = p ("canle"); c.envioRev = p ("reverb");
        c.envioEco = p ("eco"); c.bend = p ("bend");
        for (int t = 0; t < xuntos::NUM_TIPOS; ++t) c.motores[(size_t) t] = xuntos::fabrica (t).crear();
        for (auto& canle : c.soando) canle.fill (-1);
    }
    volumeParam = apvts.getRawParameterValue ("volume");
    revTam = apvts.getRawParameterValue ("revTamano");
    revBrillo = apvts.getRawParameterValue ("revBrillo");
    revPre = apvts.getRawParameterValue ("revPredelay");
    revVol = apvts.getRawParameterValue ("revVolume");
    ecoRitmo = apvts.getRawParameterValue ("ecoRitmo");
    ecoMs = apvts.getRawParameterValue ("ecoMs");
    ecoFb = apvts.getRawParameterValue ("ecoRealimentacion");
    ecoTon = apvts.getRawParameterValue ("ecoTon");
    ecoCruz = apvts.getRawParameterValue ("ecoCruzado");
    ecoVol = apvts.getRawParameterValue ("ecoVolume");

    for (int t = 0; t < xuntos::NUM_TIPOS; ++t) bancosTipo[(size_t) t] = xuntos::fabrica (t).bancos();
    // cada capa empeza co primeiro son de cada motor; despois, a primeira combinación
    for (int k = 0; k < CAPAS; ++k)
        for (int t = 0; t < xuntos::NUM_TIPOS; ++t)
        {
            const auto& b = bancosTipo[(size_t) t][0];
            ponerSon (k, t, { b.sons[0], b.sons[0], b.nomes[0], b.nome, b.descricions[0], 0, 0 });
        }
    cargarCombinacion (0);
    undo.clearUndoHistory();
    startTimerHz (30);
}

BXTProcessor::~BXTProcessor() { stopTimer(); }

juce::AudioProcessorValueTreeState::ParameterLayout BXTProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    // os interruptores e as eleccións van como eleccións (gardan o valor xa redondeado) e con texto
    // sen tiles (o texto do DAW chega ben por CLAP)
    const StringArray onOff { "Off", "On" };
    auto eleccion = [] (const String& pid, const String& nome, const StringArray& opcions, int def)
    { return std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, nome, opcions, def); };
    auto enteiro = [] (const String& pid, const String& nome, int mn, int mx, int def)
    { return std::make_unique<AudioParameterInt> (ParameterID { pid, 1 }, nome, mn, mx, def); };
    auto real = [] (const String& pid, const String& nome, float mn, float mx, float paso, float def, const char* unidade)
    { return std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, nome, NormalisableRange<float> (mn, mx, paso), def,
                                                    AudioParameterFloatAttributes().withLabel (unidade)); };
    StringArray canles { "Todas" };
    for (int i = 1; i <= 16; ++i) canles.add (String (i));
    // os valores de partida son os da primeira combinación, a que se carga ao abrir
    const auto& c0 = xuntos::COMBINACIONS[0];

    for (int k = 0; k < CAPAS; ++k)
    {
        const String c = U ("Capa ") + String (k + 1) + ": ";
        const auto& cf = c0.capas[k];
        const bool on = cf.tipo >= 0;
        auto group = std::make_unique<AudioProcessorParameterGroup> ("capa" + String (k + 1), U ("Capa ") + String (k + 1), " | ");
        group->addChild (eleccion (id (k, "on"), c + U ("acesa"), onOff, on ? 1 : 0));
        group->addChild (eleccion (id (k, "motor"), c + "motor", StringArray { "Xuno", "7FM", "FM" }, on ? cf.tipo : 0));
        group->addChild (real (id (k, "volume"), c + "volume", -60.0f, 6.0f, 0.1f, on ? cf.volume : 0.0f, "dB"));
        group->addChild (real (id (k, "pan"), c + "panorama", -1.0f, 1.0f, 0.01f, on ? cf.pan : 0.0f, ""));
        group->addChild (enteiro (id (k, "oitava"), c + "oitava", -3, 3, on ? cf.oitava : 0));
        group->addChild (enteiro (id (k, "semitons"), c + U ("semitóns"), -12, 12, on ? cf.semitons : 0));
        group->addChild (real (id (k, "cents"), c + U ("afinación fina"), -50.0f, 50.0f, 0.1f, on ? cf.cents : 0.0f, "c"));
        group->addChild (enteiro (id (k, "notaBaixa"), c + "nota baixa", 0, 127, on ? cf.notaBaixa : 0));
        group->addChild (enteiro (id (k, "notaAlta"), c + "nota alta", 0, 127, on ? cf.notaAlta : 127));
        group->addChild (enteiro (id (k, "velBaixa"), c + "velocidade baixa", 1, 127, on ? cf.velBaixa : 1));
        group->addChild (enteiro (id (k, "velAlta"), c + "velocidade alta", 1, 127, on ? cf.velAlta : 127));
        group->addChild (eleccion (id (k, "canle"), c + "canle MIDI", canles, 0));
        group->addChild (real (id (k, "reverb"), c + U ("envío á reverb"), 0.0f, 1.0f, 0.01f, on ? cf.reverb : 0.2f, ""));
        group->addChild (real (id (k, "eco"), c + U ("envío ao eco"), 0.0f, 1.0f, 0.01f, on ? cf.eco : 0.0f, ""));
        group->addChild (enteiro (id (k, "bend"), c + "alcance do bend", 0, 12, 2));
        l.add (std::move (group));
    }
    l.add (real ("volume", "Volume", -30.0f, 6.0f, 0.1f, 0.0f, "dB"));
    l.add (real ("revTamano", U ("Reverb: tamaño"), 0.0f, 1.0f, 0.01f, c0.revTamano, ""));
    l.add (real ("revBrillo", "Reverb: brillo", 0.0f, 1.0f, 0.01f, c0.revBrillo, ""));
    l.add (real ("revPredelay", "Reverb: predelay", 0.0f, 150.0f, 1.0f, c0.revPredelay, "ms"));
    l.add (real ("revVolume", "Reverb: volume", 0.0f, 1.0f, 0.01f, c0.revVolume, ""));
    l.add (eleccion ("ecoRitmo", "Eco: ritmo", StringArray { "Libre", "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" }, c0.ecoRitmo));
    l.add (real ("ecoMs", "Eco: tempo libre", 20.0f, 1500.0f, 1.0f, c0.ecoMs, "ms"));
    l.add (real ("ecoRealimentacion", U ("Eco: realimentación"), 0.0f, 0.95f, 0.01f, c0.ecoRealimentacion, ""));
    l.add (real ("ecoTon", "Eco: ton", 0.0f, 1.0f, 0.01f, c0.ecoTon, ""));
    l.add (eleccion ("ecoCruzado", "Eco: cruzado", onOff, (int) c0.ecoCruzado));
    l.add (real ("ecoVolume", "Eco: volume", 0.0f, 1.0f, 0.01f, c0.ecoVolume, ""));
    return l;
}

// ================================================================ sons das capas
int BXTProcessor::tipoCapa (int capa) const { return juce::jlimit (0, 2, (int) std::lround (capas[(size_t) capa].motor->load())); }
bool BXTProcessor::capaAcesa (int capa) const { return capas[(size_t) capa].on->load() > 0.5f; }

juce::String BXTProcessor::nomeSon (int capa, int tipo) const { return sons[(size_t) capa][(size_t) tipo].nome; }

bool BXTProcessor::editado (int capa, int tipo) const
{
    const auto& s = sons[(size_t) capa][(size_t) tipo];
    return s.son != s.orixe;
}

void BXTProcessor::ponerSon (int capa, int tipo, const SonCapa& s)
{
    sons[(size_t) capa][(size_t) tipo] = s;
    enviarSon (capa, tipo, s.son);
    cambios.fetch_add (1);
}

void BXTProcessor::escollerSon (int capa, int tipo, int banco, int indice)
{
    const auto& bs = bancosTipo[(size_t) tipo];
    if (! juce::isPositiveAndBelow (banco, (int) bs.size())) return;
    const auto& b = bs[(size_t) banco];
    if (! juce::isPositiveAndBelow (indice, (int) b.sons.size())) return;
    cambiarSons (U ("son"), [&] { ponerSon (capa, tipo, { b.sons[(size_t) indice], b.sons[(size_t) indice], b.nomes[indice], b.nome, b.descricions[indice], banco, indice }); });
}

void BXTProcessor::pasoSon (int capa, int paso)
{
    const int t = tipoCapa (capa);
    const auto& s = sons[(size_t) capa][(size_t) t];
    const auto& bs = bancosTipo[(size_t) t];
    const int b = juce::jlimit (0, (int) bs.size() - 1, s.numBanco);
    const int n = (int) bs[(size_t) b].sons.size();
    if (n > 0) escollerSon (capa, t, b, ((s.indice + paso) % n + n) % n);
}

void BXTProcessor::editarSon (int capa, int tipo, const xuntos::Son& novo)
{
    const auto antes = sons[(size_t) capa][(size_t) tipo].son;
    if (antes == novo) return;
    // a primeira vez van ao motor só os bytes que cambian (soa ao momento, sen cortar as notas);
    // ao desfacer e refacer, o son enteiro
    auto primeira = std::make_shared<bool> (true);
    estado.cambio (U ("edición"),
        [this, capa, tipo, antes, novo, primeira]
        {
            sons[(size_t) capa][(size_t) tipo].son = novo;
            if (*primeira) { *primeira = false; enviarBytes (capa, tipo, antes, novo); }
            else enviarSon (capa, tipo, novo);
            cambios.fetch_add (1);
        },
        [this, capa, tipo, antes]
        {
            sons[(size_t) capa][(size_t) tipo].son = antes;
            enviarSon (capa, tipo, antes);
            cambios.fetch_add (1);
        });
}

void BXTProcessor::restaurarSon (int capa, int tipo)
{
    if (! editado (capa, tipo)) return;
    cambiarSons (U ("restaurar"), [&]
    {
        auto s = sons[(size_t) capa][(size_t) tipo];
        s.son = s.orixe;
        ponerSon (capa, tipo, s);
    });
}

bool BXTProcessor::abrirSyx (int tipo, const juce::File& f, juce::String& mensaxe)
{
    const auto& fab = xuntos::fabrica (tipo);
    if (fab.lerSyx == nullptr) { mensaxe = U ("Este motor non le ficheiros."); return false; }
    juce::MemoryBlock mb;
    if (! f.loadFileAsData (mb) || mb.getSize() == 0 || mb.getSize() > 1024 * 1024) { mensaxe = U ("Non se puido ler o ficheiro."); return false; }
    xuntos::Banco b;
    if (! fab.lerSyx (mb, b, mensaxe)) return false;
    if (b.nome.isEmpty()) { b.nome = f.getFileNameWithoutExtension().toUpperCase().substring (0, 12); b.titulo = U ("Ficheiro: ") + f.getFileName(); }
    auto& bs = bancosTipo[(size_t) tipo];
    for (size_t i = 1; i < bs.size(); ++i)
        if (bs[i].nome == b.nome) { bs[i] = b; cambios.fetch_add (1); mensaxe = U ("Banco aberto: ") + b.nome; return true; }
    bs.push_back (b);
    cambios.fetch_add (1);
    mensaxe = U ("Banco aberto: ") + b.nome + " (" + juce::String ((int) b.sons.size()) + " sons)";
    return true;
}

// ---- da interface ao audio
void BXTProcessor::enviarSon (int capa, int tipo, const xuntos::Son& s)
{
    auto& c = capas[(size_t) capa];
    const juce::SpinLock::ScopedLockType lk (c.lock);
    c.pendente[(size_t) tipo] = s;
    c.novo[(size_t) tipo] = true;
    c.xerPendente[(size_t) tipo] = ++c.xerUI[(size_t) tipo];   // as edicións de antes xa non valen
}

void BXTProcessor::enviarBytes (int capa, int tipo, const xuntos::Son& antes, const xuntos::Son& despois)
{
    auto& c = capas[(size_t) capa];
    {
        // se aínda hai un son enteiro por enviar, mellor enviar o novo enteiro
        const juce::SpinLock::ScopedLockType lk (c.lock);
        if (c.novo[(size_t) tipo] || antes.n != despois.n) { c.pendente[(size_t) tipo] = despois; return; }
    }
    int cambian = 0;
    for (int i = 0; i < despois.n; ++i) cambian += antes.d[(size_t) i] != despois.d[(size_t) i] ? 1 : 0;
    if (cambian > c.fifo.getFreeSpace()) { enviarSon (capa, tipo, despois); return; }
    for (int i = 0; i < despois.n; ++i)
        if (antes.d[(size_t) i] != despois.d[(size_t) i])
        {
            const auto w = c.fifo.write (1);
            if (w.blockSize1 > 0) c.edicions[(size_t) w.startIndex1] = { (uint8_t) tipo, (uint8_t) i, despois.d[(size_t) i], c.xerUI[(size_t) tipo] };
        }
}

void BXTProcessor::aplicarPendentes (Capa& c)
{
    const juce::SpinLock::ScopedTryLockType lk (c.lock);
    if (! lk.isLocked()) return;   // as edicións agardan tamén: van despois do son enteiro
    for (int t = 0; t < xuntos::NUM_TIPOS; ++t)
        if (c.novo[(size_t) t])
        {
            c.motores[(size_t) t]->setSon (c.pendente[(size_t) t]);
            c.novo[(size_t) t] = false;
            c.xerAudio[(size_t) t] = c.xerPendente[(size_t) t];
        }
    const auto r = c.fifo.read (c.fifo.getNumReady());
    auto aplicar = [&c] (int inicio, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const auto& e = c.edicions[(size_t) (inicio + i)];
            if (e.xer == c.xerAudio[e.tipo]) c.motores[e.tipo]->editar (e.byte, e.valor);   // as dun son que xa non está, non
        }
    };
    aplicar (r.startIndex1, r.blockSize1);
    aplicar (r.startIndex2, r.blockSize2);
}

// ---- desfacer: os sons enteiros antes e despois
juce::ValueTree BXTProcessor::sonsEstado() const
{
    juce::ValueTree t ("Sons");
    t.setProperty ("combinacion", combinacion, nullptr);
    t.setProperty ("nomeCombinacion", nomeComb, nullptr);
    auto b64 = [] (const xuntos::Son& s) { return juce::Base64::toBase64 (s.d.data(), (size_t) s.n); };
    for (int k = 0; k < CAPAS; ++k)
    {
        juce::ValueTree c ("Capa");
        c.setProperty ("n", k, nullptr);
        for (int ti = 0; ti < xuntos::NUM_TIPOS; ++ti)
        {
            const auto& s = sons[(size_t) k][(size_t) ti];
            juce::ValueTree e ("Son");
            e.setProperty ("motor", xuntos::fabrica (ti).nome, nullptr);
            e.setProperty ("son", b64 (s.son), nullptr);
            e.setProperty ("orixe", b64 (s.orixe), nullptr);
            e.setProperty ("nome", s.nome, nullptr);
            e.setProperty ("banco", s.banco, nullptr);
            e.setProperty ("descricion", s.descricion, nullptr);
            e.setProperty ("numBanco", s.numBanco, nullptr);
            e.setProperty ("indice", s.indice, nullptr);
            c.appendChild (e, nullptr);
        }
        t.appendChild (c, nullptr);
    }
    return t;
}

void BXTProcessor::setSonsEstado (const juce::ValueTree& t)
{
    if (! t.isValid()) return;
    auto unb64 = [] (const juce::String& s, int esperado)
    {
        juce::MemoryOutputStream out;
        xuntos::Son r;
        if (! juce::Base64::convertFromBase64 (out, s) || (int) out.getDataSize() != esperado || esperado > xuntos::Son::MAX) return r;
        r.n = esperado;
        std::memcpy (r.d.data(), out.getData(), (size_t) esperado);
        return r;
    };
    combinacion = juce::jlimit (0, xuntos::NUM_COMBINACIONS - 1, (int) t.getProperty ("combinacion", 0));
    nomeComb = t.getProperty ("nomeCombinacion", U (xuntos::COMBINACIONS[combinacion].nome)).toString();
    for (const auto c : t)
    {
        const int k = c.getProperty ("n", -1);
        if (! juce::isPositiveAndBelow (k, CAPAS)) continue;
        for (const auto e : c)
        {
            int ti = -1;
            for (int x = 0; x < xuntos::NUM_TIPOS; ++x) if (e.getProperty ("motor").toString() == xuntos::fabrica (x).nome) ti = x;
            if (ti < 0) continue;
            const int bytes = sons[(size_t) k][(size_t) ti].son.n;   // o tamaño do son dese motor
            SonCapa s;
            s.son = unb64 (e.getProperty ("son").toString(), bytes);
            s.orixe = unb64 (e.getProperty ("orixe").toString(), bytes);
            if (s.son.n == 0) continue;
            if (s.orixe.n == 0) s.orixe = s.son;
            s.nome = e.getProperty ("nome").toString();
            s.banco = e.getProperty ("banco").toString();
            s.descricion = e.getProperty ("descricion").toString();
            s.numBanco = e.getProperty ("numBanco", 0);
            s.indice = e.getProperty ("indice", 0);
            ponerSon (k, ti, s);
        }
    }
    cambios.fetch_add (1);
}

// un cambio que se pode desfacer: gárdanse o antes e o despois. «facer» non se repite a primeira
// vez (o cambio xa está feito); si ao refacer.
void BXTProcessor::cambiarSons (const juce::String& nome, const std::function<void()>& f)
{
    const auto antes = sonsEstado();
    f();
    const auto despois = sonsEstado();
    if (antes.isEquivalentTo (despois)) return;
    auto primeira = std::make_shared<bool> (true);
    estado.cambio (nome, [this, despois, primeira] { if (*primeira) *primeira = false; else setSonsEstado (despois); },
                         [this, antes] { setSonsEstado (antes); });
}

escoita::Estado::Opcions BXTProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "bxtpreset";
    o.extra = [this] { return sonsEstado(); };
    o.aplicarExtra = [this] (const juce::ValueTree& t) { setSonsEstado (t); };
    return o;
}

// ================================================================ combinacións
int BXTProcessor::getNumPrograms() { return xuntos::NUM_COMBINACIONS; }
const juce::String BXTProcessor::getProgramName (int i) { return U (xuntos::COMBINACIONS[juce::jlimit (0, xuntos::NUM_COMBINACIONS - 1, i)].nome); }

void BXTProcessor::cargarCombinacion (int i)
{
    i = juce::jlimit (0, xuntos::NUM_COMBINACIONS - 1, i);
    const auto& cb = xuntos::COMBINACIONS[i];
    auto set = [this] (const juce::String& pid, float v)
    {
        if (auto* p = apvts.getParameter (pid))
        {
            const float n = p->convertTo0to1 (v);
            if (std::abs (p->getValue() - n) < 1.0e-6f) return;
            p->beginChangeGesture(); p->setValueNotifyingHost (n); p->endChangeGesture();
        }
    };
    cambiarSons (U ("combinación"), [&]
    {
        for (int k = 0; k < CAPAS; ++k)
        {
            const auto& cf = cb.capas[k];
            set (id (k, "on"), cf.tipo >= 0 ? 1.0f : 0.0f);
            if (cf.tipo < 0) continue;
            set (id (k, "motor"), (float) cf.tipo);
            set (id (k, "volume"), cf.volume); set (id (k, "pan"), cf.pan);
            set (id (k, "oitava"), (float) cf.oitava); set (id (k, "semitons"), (float) cf.semitons); set (id (k, "cents"), cf.cents);
            set (id (k, "notaBaixa"), (float) cf.notaBaixa); set (id (k, "notaAlta"), (float) cf.notaAlta);
            set (id (k, "velBaixa"), (float) cf.velBaixa); set (id (k, "velAlta"), (float) cf.velAlta);
            set (id (k, "canle"), 0.0f); set (id (k, "reverb"), cf.reverb); set (id (k, "eco"), cf.eco); set (id (k, "bend"), 2.0f);
            // o son, polo seu banco e o comezo do seu nome
            const auto& bs = bancosTipo[(size_t) cf.tipo];
            for (int b = 0; b < (int) bs.size(); ++b)
            {
                if (bs[(size_t) b].nome != U (cf.banco)) continue;
                for (int s = 0; s < bs[(size_t) b].nomes.size(); ++s)
                {
                    const auto& nome = bs[(size_t) b].nomes[s];
                    if (nome.startsWith (U (cf.son)) || nome.fromFirstOccurrenceOf (" ", false, false) == U (cf.son))
                    {
                        ponerSon (k, cf.tipo, { bs[(size_t) b].sons[(size_t) s], bs[(size_t) b].sons[(size_t) s], nome, bs[(size_t) b].nome, bs[(size_t) b].descricions[s], b, s });
                        break;
                    }
                }
                break;
            }
        }
        set ("revTamano", cb.revTamano); set ("revBrillo", cb.revBrillo); set ("revPredelay", cb.revPredelay); set ("revVolume", cb.revVolume);
        set ("ecoRitmo", (float) cb.ecoRitmo); set ("ecoMs", cb.ecoMs); set ("ecoRealimentacion", cb.ecoRealimentacion);
        set ("ecoTon", cb.ecoTon); set ("ecoCruzado", cb.ecoCruzado ? 1.0f : 0.0f); set ("ecoVolume", cb.ecoVolume);
        combinacion = i;
        nomeComb = U (cb.nome);
    });
    cambios.fetch_add (1);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void BXTProcessor::timerCallback()
{
    const int c = combinacionPendente.exchange (-1);
    if (c >= 0) cargarCombinacion (c);
}

// ================================================================ audio
bool BXTProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BXTProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    setLatencySamples (0);
    const size_t n = (size_t) juce::jmax (1, samplesPerBlock);
    for (auto& c : capas)
    {
        for (auto& m : c.motores) m->preparar (sampleRate);
        aplicarPendentes (c);
        for (auto& m : c.motores) m->calar();
        for (auto& canle : c.soando) canle.fill (-1);
        c.tipo = -1; c.bendLast = -1000; c.centsLast = -1000; c.roda14 = 8192;
        c.L.assign (n, 0.0f); c.R.assign (n, 0.0f);
        c.gL = c.gR = 0;
    }
    revL.assign (n, 0.0f); revR.assign (n, 0.0f); ecoL.assign (n, 0.0f); ecoR.assign (n, 0.0f);
    reverb.preparar (sampleRate);
    eco.preparar (sampleRate);
    teito.preparar (sampleRate, 0.080, 0.891);
    volS = juce::Decibels::decibelsToGain ((double) volumeParam->load());
    keyboardState.reset();
}

void BXTProcessor::soltarCapa (Capa& c, bool calar)
{
    if (c.tipo >= 0)
    {
        auto* m = c.motores[(size_t) c.tipo].get();
        if (calar) m->calar(); else m->soltarTodas();
    }
    for (auto& canle : c.soando) canle.fill (-1);
}

void BXTProcessor::midiCapa (Capa& c, int indice, const juce::MidiMessage& m)
{
    if (c.tipo < 0) return;
    const int ch = juce::jlimit (1, 16, m.getChannel());
    const int filtro = (int) std::lround (c.canle->load());
    if (filtro > 0 && filtro != ch && ! m.isAllNotesOff() && ! m.isAllSoundOff()) return;
    auto* mo = c.motores[(size_t) c.tipo].get();
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber(), v = m.getVelocity();
        auto& s = c.soando[(size_t) ch - 1][(size_t) n];
        if (s >= 0) { mo->noteOff (s); s = -1; }
        if (n < (int) c.notaBaixa->load() || n > (int) c.notaAlta->load() || v < (int) c.velBaixa->load() || v > (int) c.velAlta->load()) return;
        const int out = n + 12 * (int) std::lround (c.oitava->load()) + (int) std::lround (c.semitons->load());
        if (out < 0 || out > 127) return;
        mo->noteOn (out, v);
        s = (int8_t) out;
        notasCapa[(size_t) indice].fetch_add (1);
    }
    else if (m.isNoteOff())
    {
        auto& s = c.soando[(size_t) ch - 1][(size_t) m.getNoteNumber()];
        if (s >= 0) { mo->noteOff (s); s = -1; }
    }
    else if (m.isSustainPedalOn()) mo->pedal (true);
    else if (m.isSustainPedalOff()) mo->pedal (false);
    else if (m.isPitchWheel()) { c.roda14 = m.getPitchWheelValue(); mo->bend (c.roda14); }
    else if (m.isControllerOfType (1)) mo->roda (m.getControllerValue());
    else if (m.isAllSoundOff()) soltarCapa (c, true);
    else if (m.isAllNotesOff()) soltarCapa (c, false);
    else if (m.isResetAllControllers()) { c.roda14 = 8192; mo->bend (8192); mo->roda (0); mo->pedal (false); }
}

void BXTProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
            if (const auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 400.0, *b);

    // os efectos
    reverb.axustar (revTam->load(), revBrillo->load(), revPre->load());
    {
        const int ritmo = juce::jlimit (0, 8, (int) std::lround (ecoRitmo->load()));
        const double ms = ritmo == 0 ? (double) ecoMs->load() : RITMOS[ritmo] * 60000.0 / bpm;
        eco.axustar (ms, ecoFb->load(), ecoTon->load(), ecoCruz->load() > 0.5f);
    }

    // as capas: o seu son, o seu motor e a súa afinación
    const auto tamano = (size_t) n;
    if (revL.size() < tamano) { revL.resize (tamano); revR.resize (tamano); ecoL.resize (tamano); ecoR.resize (tamano); }   // por se o DAW manda máis do anunciado
    for (auto& c : capas)
    {
        aplicarPendentes (c);
        const int t = c.on->load() > 0.5f ? juce::jlimit (0, 2, (int) std::lround (c.motor->load())) : -1;
        if (t != c.tipo)
        {
            soltarCapa (c, true);
            c.tipo = t;
            c.bendLast = -1000; c.centsLast = -1000;
            if (t >= 0) c.motores[(size_t) t]->bend (c.roda14);
        }
        if (c.tipo >= 0)
        {
            auto* mo = c.motores[(size_t) c.tipo].get();
            const int b = (int) std::lround (c.bend->load());
            const double ct = c.cents->load();
            if (b != c.bendLast) { mo->rangoBend (b); c.bendLast = b; }
            if (std::abs (ct - c.centsLast) > 1.0e-9) { mo->afinar (ct); c.centsLast = ct; }
        }
        if (c.L.size() < tamano) { c.L.resize (tamano); c.R.resize (tamano); }
    }

    // o MIDI, cada evento na súa mostra
    auto renderizar = [this] (int a, int b)
    {
        for (auto& c : capas)
            if (c.tipo >= 0) c.motores[(size_t) c.tipo]->render (c.L.data() + a, c.R.data() + a, b - a);
    };
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { renderizar (pos, t); pos = t; }
        const auto m = meta.getMessage();
        if (m.isProgramChange()) { combinacionPendente.store (m.getProgramChangeNumber()); continue; }
        for (int k = 0; k < CAPAS; ++k) midiCapa (capas[(size_t) k], k, m);
    }
    if (pos < n) renderizar (pos, n);

    // a mestura: cada capa co seu volume e panorama, e os seus envíos
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    std::fill (outL, outL + n, 0.0f);
    if (outR != nullptr) std::fill (outR, outR + n, 0.0f);
    std::fill (revL.begin(), revL.begin() + n, 0.0f); std::fill (revR.begin(), revR.begin() + n, 0.0f);
    std::fill (ecoL.begin(), ecoL.begin() + n, 0.0f); std::fill (ecoR.begin(), ecoR.begin() + n, 0.0f);
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    for (int k = 0; k < CAPAS; ++k)
    {
        auto& c = capas[(size_t) k];
        if (c.tipo < 0) { vocesCapa[(size_t) k].store (0); nivelCapa[(size_t) k].store (0); c.gL = c.gR = 0; continue; }
        const double g = juce::Decibels::decibelsToGain ((double) c.volume->load(), -59.9) * xuntos::fabrica (c.tipo).ganancia * MESTURA;
        const double p = c.pan->load();
        const double tL = g * (p > 0 ? 1.0 - p : 1.0), tR = g * (p < 0 ? 1.0 + p : 1.0);
        const float er = c.envioRev->load(), ee = c.envioEco->load();
        float pico = 0;
        for (int i = 0; i < n; ++i)
        {
            c.gL += (tL - c.gL) * sm; c.gR += (tR - c.gR) * sm;
            const float l = (float) (c.L[(size_t) i] * c.gL), r = (float) (c.R[(size_t) i] * c.gR);
            outL[i] += l;
            if (outR != nullptr) outR[i] += r; else outL[i] += r;
            revL[(size_t) i] += l * er; revR[(size_t) i] += r * er;
            ecoL[(size_t) i] += l * ee; ecoR[(size_t) i] += r * ee;
            pico = std::max ({ pico, std::abs (l), std::abs (r) });
        }
        vocesCapa[(size_t) k].store (c.motores[(size_t) c.tipo]->vocesActivas());
        nivelCapa[(size_t) k].store (pico);
    }

    // os efectos, o volume e o limitador de seguridade do núcleo (teito a −1 dBFS, sen latencia)
    const double volT = juce::Decibels::decibelsToGain ((double) volumeParam->load());
    const float rv = revVol->load(), ev = ecoVol->load();
    float pico = 0;
    for (int i = 0; i < n; ++i)
    {
        float rl = revL[(size_t) i], rr = revR[(size_t) i], el = ecoL[(size_t) i], er = ecoR[(size_t) i];
        reverb.procesar (rl, rr);
        eco.procesar (el, er);
        volS += (volT - volS) * sm;
        double l = (outL[i] + rl * rv + el * ev) * volS;
        double r = ((outR != nullptr ? outR[i] : outL[i]) + rr * rv + er * ev) * volS;
        if (outR != nullptr) { teito.procesar (l, r); outL[i] = (float) l; outR[i] = (float) r; }
        else outL[i] = (float) teito.procesar (0.5 * (l + r));
        pico = std::max ({ pico, std::abs ((float) l), std::abs ((float) r) });
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    nivelSaida.store (pico);
}

// ================================================================ estado
void BXTProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.removeChild (st.getChildWithName ("Sons"), nullptr);
    st.appendChild (sonsEstado(), nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BXTProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    const auto s = tree.getChildWithName ("Sons");
    setSonsEstado (s);
    tree.removeChild (s, nullptr);
    apvts.replaceState (tree);   // (e o desfacer empeza de novo)
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

juce::AudioProcessorEditor* BXTProcessor::createEditor() { return new BXTEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BXTProcessor(); }
#endif
