// Proba sen interface: os tres motores e todos os seus sons, as capas (zonas, velocidade, oitava,
// canle), os efectos, as combinacións, a edición e o desfacer, o estado, o A/B e os presets, o
// limitador e a independencia do tamaño de bloque; tamén capturas da interface.
//   BXTRenderTest [saida.wav]                         proba enteira (devolve o número de fallos)
//   BXTRenderTest --snapshot saida.png [capa a editar]
//   BXTRenderTest --render saida.wav [combinación]    unha combinación: un acorde e unha frase
//   BXTRenderTest --niveis                            a sonoridade dos primeiros sons de cada motor
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Combinacions.h"
#include <cstdio>
#include <cstring>

using escoita::U;

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

struct Ev { double t; int type; int a; int b; int ch = 1; };   // 0 on, 1 off, 3 cc, 5 bend

static juce::AudioBuffer<float> play (BXTProcessor& p, std::vector<Ev> seq, double len, int blockSize, double sr, bool preparar = true)
{
    std::stable_sort (seq.begin(), seq.end(), [] (const Ev& x, const Ev& y) { return x.t < y.t; });   // na orde do tempo
    if (preparar) { p.setPlayConfigDetails (0, 2, sr, blockSize); p.prepareToPlay (sr, blockSize); }
    const int total = (int) (len * sr);
    juce::AudioBuffer<float> out (2, total), buf (2, blockSize);
    size_t k = 0;
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total - pos);
        juce::MidiBuffer midi;
        while (k < seq.size() && (int) (seq[k].t * sr) < pos + n)
        {
            const int at = juce::jmax (0, (int) (seq[k].t * sr) - pos);
            const auto& e = seq[k++];
            juce::MidiMessage m = e.type == 0 ? juce::MidiMessage::noteOn (e.ch, e.a, (juce::uint8) e.b)
                               : e.type == 1 ? juce::MidiMessage::noteOff (e.ch, e.a)
                               : e.type == 3 ? juce::MidiMessage::controllerEvent (e.ch, e.a, e.b)
                               : juce::MidiMessage::pitchWheel (e.ch, e.a);
            midi.addEvent (m, at);
        }
        buf.setSize (2, n, false, false, true);
        buf.clear();
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    return out;
}

static void setParam (BXTProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// só unha capa, cun motor e o seu son de banco
static void soCapa (BXTProcessor& p, int tipo, int banco, int son)
{
    for (int k = 0; k < BXTProcessor::CAPAS; ++k) setParam (p, BXTProcessor::id (k, "on"), k == 0 ? 1.0f : 0.0f);
    setParam (p, BXTProcessor::id (0, "motor"), (float) tipo);
    for (auto e : { "oitava", "semitons", "cents", "pan", "volume", "reverb", "eco" }) setParam (p, BXTProcessor::id (0, e), 0.0f);
    setParam (p, BXTProcessor::id (0, "notaBaixa"), 0); setParam (p, BXTProcessor::id (0, "notaAlta"), 127);
    setParam (p, BXTProcessor::id (0, "velBaixa"), 1); setParam (p, BXTProcessor::id (0, "velAlta"), 127);
    p.escollerSon (0, tipo, banco, son);
}

static std::vector<Ev> acorde (double t0 = 0.02, double dur = 0.8, int vel = 100)
{
    std::vector<Ev> s;
    for (int n : { 48, 60, 64, 67 }) s.push_back ({ t0, 0, n, vel });
    for (int n : { 48, 60, 64, 67 }) s.push_back ({ t0 + dur, 1, n, 0 });
    return s;
}

static float pico (const juce::AudioBuffer<float>& b) { return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples())); }
static bool finito (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c) for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
static float diferenza (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c) for (int i = 0; i < juce::jmin (a.getNumSamples(), b.getNumSamples()); ++i) d = juce::jmax (d, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return d;
}
// o máximo do valor eficaz en ventás de 100 ms (sonoridade a curto prazo), en dB
static double sonoridade (const juce::AudioBuffer<float>& o, double sr)
{
    const int w = (int) (0.1 * sr);
    double best = 0;
    for (int a = 0; a + w <= o.getNumSamples(); a += w / 4)
    {
        double e = 0;
        for (int s = a; s < a + w; ++s) e += 0.5 * (o.getSample (0, s) * o.getSample (0, s) + o.getSample (1, s) * o.getSample (1, s));
        best = std::max (best, e / w);
    }
    return 10.0 * std::log10 (best + 1e-12);
}
// a frecuencia por cruces por cero (sinal periódico)
static double frecuencia (const juce::AudioBuffer<float>& b, double sr, int a, int z)
{
    double first = -1, last = -1; int cnt = 0;
    for (int i = a + 1; i < z; ++i)
        if (b.getSample (0, i - 1) < 0 && b.getSample (0, i) >= 0)
        {
            const double t = (double) (i - 1) + (double) b.getSample (0, i - 1) / (double) (b.getSample (0, i - 1) - b.getSample (0, i));
            if (first < 0) first = t; last = t; ++cnt;
        }
    return cnt > 1 ? (cnt - 1) * sr / (last - first) : 0.0;
}
// a pegada dun son: FNV-1a de 64 bits das mostras (un cambio de organización non pode movela nin un bit)
static uint64_t pegada (const juce::AudioBuffer<float>& b)
{
    uint64_t h = 1469598103934665603ull;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            uint32_t u; std::memcpy (&u, &x, 4);
            for (int k = 0; k < 4; ++k) { h ^= (u >> (8 * k)) & 0xFF; h *= 1099511628211ull; }
        }
    return h;
}

static void snapshot (const juce::File& png, int editar)
{
    BXTProcessor p;
    p.prepareToPlay (48000, 512);
    p.cargarCombinacion (13);   // Catro capas
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<BXTEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    if (editar >= 0) ce->view().openEditor (editar);
    for (int n : { 36, 60, 64, 67 }) p.keyboardState.noteOn (1, n, 1.0f);
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 30; ++blk) { juce::MidiBuffer m; buf.clear(); p.processBlock (buf, m); p.serviceMessages(); ce->view().timerCallback(); }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 2) { snapshot (juce::File (juce::String (argv[2])), argc > 3 ? atoi (argv[3]) : -1); return 0; }

    if (mode == "--render" && argc > 2)
    {
        BXTProcessor p;
        p.cargarCombinacion (argc > 3 ? atoi (argv[3]) : 0);
        auto seq = acorde (0.05, 1.6);
        const int frase[] = { 60, 62, 64, 67, 69, 67, 64, 62, 60, 55, 48, 36 };
        for (int i = 0; i < 12; ++i) { seq.push_back ({ 2.4 + i * 0.3, 0, frase[i], 60 + i * 5 }); seq.push_back ({ 2.4 + i * 0.3 + 0.26, 1, frase[i], 0 }); }
        writeWav (juce::File (juce::String (argv[2])), play (p, seq, 8.5, 512, sr), sr);
        return 0;
    }

    if (mode == "--niveis")
    {
        for (int t = 0; t < xuntos::NUM_TIPOS; ++t)
        {
            double suma = 0; int n = 0;
            BXTProcessor p;
            const auto& b = p.bancos (t)[0];
            for (int i = 0; i < (int) b.sons.size(); i += juce::jmax (1, (int) b.sons.size() / 24))
            {
                soCapa (p, t, 0, i);
                const double l = sonoridade (play (p, acorde (0.02, 0.6), 0.8, 512, sr), sr);
                suma += l; ++n;
            }
            std::printf ("%-5s media %.1f dB (%d sons)\n", xuntos::fabrica (t).nome, suma / n, n);
        }
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const char* what, const juce::String& extra = {})
    {
        std::printf ("%-52s %s %s\n", what, ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    {   // os parámetros, nos seus valores de partida, salvo os que pon a primeira combinación
        BXTProcessor p;
        bool ok = p.getCombinacion() == 0 && p.nomeCombinacion() == U (xuntos::COMBINACIONS[0].nome) && ! p.undo.canUndo();
        juce::String mal;   // e cada parámetro no seu valor de partida (a primeira combinación é a de partida)
        for (auto* prm : p.getParameters())
            if (std::abs (prm->getValue() - prm->getDefaultValue()) > 1.0e-6f) mal << prm->getName (40) << " ";
        check (ok && mal.isEmpty(), "ao abrir: a primeira combinación, nos valores de partida", mal);
    }
    {   // os tres motores e todos os seus sons: soan e sen valores non numéricos
        for (int t = 0; t < xuntos::NUM_TIPOS; ++t)
        {
            BXTProcessor p;
            int total = 0, calados = 0; bool nan = false;
            for (int b = 0; b < (int) p.bancos (t).size(); ++b)
                for (int i = 0; i < (int) p.bancos (t)[(size_t) b].sons.size(); ++i)
                {
                    soCapa (p, t, b, i);
                    std::vector<Ev> s { { 0.01, 0, 60, 100 }, { 0.30, 1, 60, 0 } };
                    const auto o = play (p, s, 0.4, 512, sr);
                    nan = nan || ! finito (o);
                    calados += pico (o) < 1.0e-4f ? 1 : 0;
                    ++total;
                }
            const juce::String que = juce::String (xuntos::fabrica (t).nome) + ": " + juce::String (total) + " sons, " + juce::String (calados) + " calados";
            check (! nan && calados <= total / 20, (juce::String ("motor ") + xuntos::fabrica (t).nome + ": todos os sons").toRawUTF8(), "(" + que + ")");
        }
    }
    {   // zonas: «Baixo e teclas» parte no Do3
        BXTProcessor p;
        p.cargarCombinacion (3);
        const uint32_t a0 = p.notasCapa[0].load(), b0 = p.notasCapa[1].load();
        play (p, { { 0.01, 0, 40, 100 }, { 0.1, 1, 40, 0 } }, 0.2, 512, sr);
        const bool baixo = p.notasCapa[0].load() == a0 + 1 && p.notasCapa[1].load() == b0;
        play (p, { { 0.01, 0, 72, 100 }, { 0.1, 1, 72, 0 } }, 0.2, 512, sr, false);
        const bool alto = p.notasCapa[0].load() == a0 + 1 && p.notasCapa[1].load() == b0 + 1;
        check (baixo && alto, "zonas: cada nota á súa capa");
    }
    {   // velocidade: en «Toca forte», as campás só tocando forte
        BXTProcessor p;
        p.cargarCombinacion (6);
        const uint32_t c0 = p.notasCapa[1].load();
        play (p, { { 0.01, 0, 60, 60 }, { 0.1, 1, 60, 0 } }, 0.2, 512, sr);
        const bool suave = p.notasCapa[1].load() == c0;
        play (p, { { 0.01, 0, 60, 120 }, { 0.1, 1, 60, 0 } }, 0.2, 512, sr, false);
        check (suave && p.notasCapa[1].load() == c0 + 1, "velocidade: a capa forte só entra forte");
    }
    {   // oitava: a mesma nota, unha oitava por riba (o son de partida do Juno: serra aberta)
        auto f = [&] (int oit)
        {
            BXTProcessor p;
            soCapa (p, xuntos::XUNO, 0, 127);   // B88 Inicial
            setParam (p, BXTProcessor::id (0, "reverb"), 0.0f);
            setParam (p, BXTProcessor::id (0, "oitava"), (float) oit);
            const auto o = play (p, { { 0.01, 0, 57, 100 } }, 0.5, 512, sr);
            return frecuencia (o, sr, (int) (0.2 * sr), (int) (0.45 * sr));
        };
        const double f0 = f (0), f1 = f (1);
        check (std::abs (f1 / f0 - 2.0) < 0.01 && std::abs (f0 - 220.0) < 1.5, "oitava: o La2 a 220 Hz e unha oitava por riba",
               juce::String::formatted ("(%.2f Hz e %.2f Hz)", f0, f1));
    }
    {   // canle MIDI: unha capa na canle 2 non escoita a 1
        BXTProcessor p;
        soCapa (p, xuntos::XUNO, 0, 0);
        setParam (p, BXTProcessor::id (0, "canle"), 2.0f);
        const uint32_t c0 = p.notasCapa[0].load();
        play (p, { { 0.01, 0, 60, 100, 1 }, { 0.05, 1, 60, 0, 1 } }, 0.1, 512, sr);
        const bool ignora = p.notasCapa[0].load() == c0;
        play (p, { { 0.01, 0, 60, 100, 2 }, { 0.05, 1, 60, 0, 2 } }, 0.1, 512, sr, false);
        check (ignora && p.notasCapa[0].load() == c0 + 1, "canle MIDI: cada capa a súa");
    }
    {   // tamaño de bloque: as catro capas, os tres motores e os efectos, iguais con 512 e con 37 mostras
        BXTProcessor a, b;
        a.cargarCombinacion (13); b.cargarCombinacion (13);
        auto seq = acorde (0.05, 1.5, 90);
        seq.push_back ({ 0.7, 5, 12000, 0 }); seq.push_back ({ 1.0, 5, 8192, 0 }); seq.push_back ({ 1.1, 3, 64, 127 }); seq.push_back ({ 2.0, 3, 64, 0 });
        const auto x = play (a, seq, 3.0, 512, sr), y = play (b, seq, 3.0, 37, sr);
        const float d = diferenza (x, y);
        check (d < 1.0e-6f && finito (x) && pico (x) > 0.01f, "bloques de 512 e de 37 mostras: iguais", juce::String::formatted ("(diferenza %g)", d));
        std::printf ("pegada da saída: %016llx\n", (unsigned long long) pegada (x));
        if (argc > 1 && ! mode.startsWith ("--")) writeWav (juce::File (mode), x, sr);
    }
    {   // edición: un cambio no son soa; desfeito, o son volve a ser o do banco, bit a bit
        auto render = [&] (bool editar, bool desfacer, bool vivo)
        {
            BXTProcessor p;
            soCapa (p, xuntos::DX, 1, 0);   // 7FM BARULLO 01 PIANO ELEC
            p.undo.clearUndoHistory();
            // vivo: o motor xa está a soar cando chega a edición (vai polos bytes que cambian)
            if (vivo) { p.setPlayConfigDetails (0, 2, sr, 512); p.prepareToPlay (sr, 512); juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; p.processBlock (buf, m); }
            if (editar)
            {
                p.estado.novoPaso();
                auto s = p.son (0, xuntos::DX);
                s.d[134] = (uint8_t) ((s.d[134] + 7) % 32);   // outro algoritmo
                p.editarSon (0, xuntos::DX, s);
                p.estado.novoPaso();
            }
            if (desfacer) p.undo.undo();
            return play (p, acorde (0.02, 0.5), 0.8, 512, sr, ! vivo);
        };
        const auto orixinal = render (false, false, false), editado = render (true, false, false), desfeito = render (true, true, false);
        const auto vivoSen = render (false, false, true), vivoCon = render (true, false, true);
        const float dEd = diferenza (orixinal, editado), dDesf = diferenza (orixinal, desfeito), dVivo = diferenza (vivoSen, vivoCon);
        check (dEd > 0.01f && dDesf == 0.0f && dVivo > 0.01f, "edición: soa (antes e en directo) e desfaise",
               juce::String::formatted ("(%.3f, %.3f, %.3f)", dEd, dDesf, dVivo));
    }
    {   // estado: os parámetros e os sons (editados) gárdanse e recupéranse; soa igual
        BXTProcessor a;
        a.cargarCombinacion (2);
        setParam (a, BXTProcessor::id (1, "volume"), -3.0f);
        auto s = a.son (0, xuntos::DX); s.d[16] = 50; a.editarSon (0, xuntos::DX, s);
        a.escollerSon (2, xuntos::FM, 0, 23);
        a.uiScale = 0.8f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        BXTProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool ok = b.son (0, xuntos::DX) == a.son (0, xuntos::DX) && b.editado (0, xuntos::DX) && b.son (2, xuntos::FM) == a.son (2, xuntos::FM)
                  && b.nomeSon (2, xuntos::FM) == a.nomeSon (2, xuntos::FM) && std::abs (b.uiScale - 0.8f) < 1e-6f && b.getCombinacion() == 2;
        const auto x = play (a, acorde(), 1.2, 512, sr), y = play (b, acorde(), 1.2, 512, sr);
        ok = ok && diferenza (x, y) == 0.0f;
        check (ok, "estado gardado/recuperado (soa igual)", "(" + juce::String ((int) mb.getSize()) + " bytes)");
    }
    {   // A/B e presets propios (do núcleo), cos sons das capas
        BXTProcessor p;
        p.cargarCombinacion (0);
        const auto a0 = p.son (1, xuntos::XUNO);
        p.estado.cambiarLado();
        p.cargarCombinacion (10);
        const auto b0 = p.son (1, xuntos::XUNO);
        p.estado.cambiarLado();
        bool ok = p.estado.lado() == 0 && p.son (1, xuntos::XUNO) == a0 && p.getCombinacion() == 0;
        p.estado.cambiarLado();
        ok = ok && p.son (1, xuntos::XUNO) == b0 && p.getCombinacion() == 10 && a0 != b0;
        check (ok, "A/B (capas, sons e combinación)");

        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("barulloxuntos_presets_proba");
        dir.deleteRecursively();
        p.estado.setCarpeta (dir);
        auto s = p.son (0, xuntos::XUNO); s.d[5] = 99; p.editarSon (0, xuntos::XUNO, s);
        const bool saved = p.estado.gardar ("Proba: capas");
        const auto gardado = p.son (0, xuntos::XUNO);
        p.cargarCombinacion (3);
        const auto list = p.estado.presets();
        const bool loaded = list.size() == 1 && list[0].hasFileExtension ("bxtpreset") && p.estado.cargar (list[0]);
        check (saved && loaded && p.son (0, xuntos::XUNO) == gardado && p.getCombinacion() == 10, "presets propios: gardar e cargar");
        dir.deleteRecursively();
    }
    {   // desfacer e refacer: escoller un son e cargar unha combinación, cada un no seu paso
        BXTProcessor p;
        p.cargarCombinacion (0);
        p.undo.clearUndoHistory();
        const auto s0 = p.son (1, xuntos::XUNO);
        p.estado.novoPaso(); p.escollerSon (1, xuntos::XUNO, 0, 40); p.estado.novoPaso();
        const auto s1 = p.son (1, xuntos::XUNO);
        p.cargarCombinacion (10); p.apvts.copyState(); p.estado.novoPaso();
        p.undo.undo();
        const bool u1 = p.getCombinacion() == 0 && p.son (1, xuntos::XUNO) == s1;
        p.undo.undo();
        const bool u2 = p.son (1, xuntos::XUNO) == s0;
        p.undo.redo(); p.undo.redo();
        check (u1 && u2 && p.getCombinacion() == 10, "desfacer e refacer (son e combinación)");
    }
    {   // as combinacións: todas soan
        BXTProcessor p;
        int mal = 0;
        for (int i = 0; i < xuntos::NUM_COMBINACIONS; ++i)
        {
            p.cargarCombinacion (i);
            const auto o = play (p, acorde (0.02, 0.6), 0.9, 512, sr);
            if (! finito (o) || pico (o) < 0.01f) { ++mal; std::printf ("   calada: %s\n", xuntos::COMBINACIONS[i].nome); }
        }
        check (mal == 0, "as combinacións soan", "(" + juce::String (xuntos::NUM_COMBINACIONS) + ")");
    }
    {   // o eco á par do DAW: a corchea a 120 BPM (sen DAW) son 250 ms; a reverb apágase
        xuntos::Eco e; e.preparar (sr); e.axustar (250.0, 0.0, 1.0, false);
        int onde = -1; float mx = 0;
        for (int i = 0; i < (int) sr; ++i) { float l = i == 0 ? 1.0f : 0.0f, r = l; e.procesar (l, r); if (std::abs (l) > mx) { mx = std::abs (l); onde = i; } }
        xuntos::Reverb rv; rv.preparar (sr); rv.axustar (0.3, 0.6, 0.0);
        double cedo = 0, tarde = 0;
        for (int i = 0; i < (int) (3 * sr); ++i)
        {
            float l = i == 0 ? 1.0f : 0.0f, r = l;
            rv.procesar (l, r);
            const double en = (double) l * l + (double) r * r;
            if (i > 0.05 * sr && i < 0.15 * sr) cedo += en;
            if (i > 2.5 * sr) tarde += en;
        }
        const double caida = 10.0 * std::log10 ((tarde + 1e-30) / (cedo + 1e-30));
        check (std::abs (onde - (int) (0.25 * sr)) <= 1 && caida < -40.0 && cedo > 1e-4, "eco no seu tempo e reverb que se apaga",
               juce::String::formatted ("(%d mostras; %.0f dB)", onde, caida));
    }
    {   // o limitador do núcleo: catro capas a +6 dB e un acorde forte non pasan de −1 dBFS
        BXTProcessor p;
        p.cargarCombinacion (13);
        for (int k = 0; k < BXTProcessor::CAPAS; ++k) setParam (p, BXTProcessor::id (k, "volume"), 6.0f);
        setParam (p, "volume", 6.0f);
        std::vector<Ev> s;
        for (int n : { 36, 43, 48, 52, 55, 60, 64, 67, 72 }) s.push_back ({ 0.02, 0, n, 127 });
        const float pk = pico (play (p, s, 1.5, 512, sr));
        check (pk <= 0.8913f, "teito a -1 dBFS", juce::String::formatted ("(pico %.4f)", pk));
    }
    return fails;
}
