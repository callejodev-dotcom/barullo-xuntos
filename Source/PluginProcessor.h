#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "motores/Motor.h"
#include "Efectos.h"

// Barullo Xuntos: catro capas e en cada unha un dos motores de Barullo (o Juno-106 de Xuno, o DX7
// de 7FM ou o chip do teclado de Barullo FM), co seu son, a súa zona do teclado e da velocidade, o
// seu volume, panorama e afinación, e os envíos a dous efectos comúns (reverb e eco). O que se
// garda é unha combinación: os parámetros (capas e efectos) e os sons de cada capa, editados ou non.
class BXTProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr int CAPAS = 4;

    BXTProcessor();
    ~BXTProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo Xuntos"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    // os programas son as combinacións de fábrica
    int getNumPrograms() override;
    int getCurrentProgram() override { return combinacion; }
    void setCurrentProgram (int index) override { cargarCombinacion (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String id (int capa, const char* nome) { return "c" + juce::String (capa + 1) + "_" + nome; }

    juce::UndoManager undo;   // desfacer e refacer (declárase antes ca o apvts)
    juce::AudioProcessorValueTreeState apvts;
    // presets propios (.bxtpreset en Documentos/Barullo Xuntos/Presets), A/B e desfacer, do núcleo
    // común: os parámetros e os sons das capas
    escoita::Estado estado { apvts, &undo, opcionsEstado() };
    juce::MidiKeyboardState keyboardState;

    // ---- os sons das capas (só desde o fío da interface) ----
    int tipoCapa (int capa) const;
    bool capaAcesa (int capa) const;
    const xuntos::Son& son (int capa, int tipo) const { return sons[(size_t) capa][(size_t) tipo].son; }
    juce::String nomeSon (int capa, int tipo) const;          // «A21 Cordas», «01 PIANO ELEC»…
    juce::String bancoSon (int capa, int tipo) const { return sons[(size_t) capa][(size_t) tipo].banco; }
    juce::String descricionSon (int capa, int tipo) const { return sons[(size_t) capa][(size_t) tipo].descricion; }
    bool editado (int capa, int tipo) const;
    const std::vector<xuntos::Banco>& bancos (int tipo) const { return bancosTipo[(size_t) tipo]; }
    void escollerSon (int capa, int tipo, int banco, int indice);   // dun banco (pódese desfacer)
    void editarSon (int capa, int tipo, const xuntos::Son& novo);    // unha edición: van ao motor só os bytes que cambian
    void restaurarSon (int capa, int tipo);                          // volve ao son do banco
    bool abrirSyx (int tipo, const juce::File&, juce::String& mensaxe);   // engade un banco .syx á lista do motor
    void pasoSon (int capa, int paso);                               // o son seguinte ou o anterior do mesmo banco

    // ---- combinacións ----
    void cargarCombinacion (int i);
    int getCombinacion() const { return combinacion; }
    juce::String nomeCombinacion() const { return nomeComb; }

    // cambia cada vez que muda un son ou unha combinación (a interface mira isto)
    std::atomic<uint32_t> cambios { 1 };

    // ---- para a pantalla (escríbese desde o fío de audio) ----
    std::array<std::atomic<int>, CAPAS> vocesCapa {};
    std::array<std::atomic<float>, CAPAS> nivelCapa {};
    std::atomic<float> nivelSaida { 0 };
    std::array<std::atomic<uint32_t>, CAPAS> notasCapa {};   // contador de notas que entraron en cada capa
    float uiScale = 1.0f;

    void serviceMessages() { timerCallback(); }   // o mesmo que fai o temporizador (para as probas)

private:
    // o son dunha capa para un motor: o actual (quizais editado), o do banco e de onde vén
    struct SonCapa
    {
        xuntos::Son son, orixe;
        juce::String nome, banco, descricion;
        int numBanco = 0, indice = 0;
    };
    std::array<std::array<SonCapa, xuntos::NUM_TIPOS>, CAPAS> sons;
    std::array<std::vector<xuntos::Banco>, xuntos::NUM_TIPOS> bancosTipo;
    int combinacion = 0;
    juce::String nomeComb;

    void ponerSon (int capa, int tipo, const SonCapa&);   // sen desfacer: á interface e ao audio
    juce::ValueTree sonsEstado() const;                    // os sons das capas nun ValueTree (o mesmo do proxecto)
    void setSonsEstado (const juce::ValueTree&);
    void cambiarSons (const juce::String& nome, const std::function<void()>& f);   // un cambio que se pode desfacer
    escoita::Estado::Opcions opcionsEstado();

    // ---- fío de audio ----
    struct Edicion { uint8_t tipo = 0, byte = 0, valor = 0; uint32_t xer = 0; };
    struct Capa
    {
        std::array<std::unique_ptr<xuntos::Motor>, xuntos::NUM_TIPOS> motores;
        // da interface ao audio: sons enteiros (cun cadeado) e bytes editados (sen cadeado)
        juce::SpinLock lock;
        std::array<xuntos::Son, xuntos::NUM_TIPOS> pendente;
        std::array<bool, xuntos::NUM_TIPOS> novo {};
        // cada son enteiro leva un número: unha edición só vale para o son co que se fixo
        std::array<uint32_t, xuntos::NUM_TIPOS> xerUI {}, xerPendente {}, xerAudio {};
        juce::AbstractFifo fifo { 1024 };
        std::array<Edicion, 1024> edicions;
        // estado do audio
        int tipo = -1;                                  // o motor que soa (−1: capa apagada)
        std::array<std::array<int8_t, 128>, 16> soando; // [canle][nota que entrou] → a nota que soa (−1)
        int bendLast = -1000; double centsLast = -1000; int roda14 = 8192;
        std::vector<float> L, R;
        double gL = 0, gR = 0;
        // parámetros
        std::atomic<float> *on = nullptr, *motor = nullptr, *volume = nullptr, *pan = nullptr, *oitava = nullptr, *semitons = nullptr,
                           *cents = nullptr, *notaBaixa = nullptr, *notaAlta = nullptr, *velBaixa = nullptr, *velAlta = nullptr,
                           *canle = nullptr, *envioRev = nullptr, *envioEco = nullptr, *bend = nullptr;
    };
    std::array<Capa, CAPAS> capas;
    void enviarSon (int capa, int tipo, const xuntos::Son&);
    void enviarBytes (int capa, int tipo, const xuntos::Son& antes, const xuntos::Son& despois);
    void aplicarPendentes (Capa&);
    void midiCapa (Capa&, int indice, const juce::MidiMessage&);
    void soltarCapa (Capa&, bool calar);

    xuntos::Reverb reverb;
    xuntos::Eco eco;
    escoita::Teito teito;
    std::vector<float> revL, revR, ecoL, ecoR;
    double sr = 48000, volS = 1, bpm = 120;
    std::atomic<float> *volumeParam = nullptr, *revTam = nullptr, *revBrillo = nullptr, *revPre = nullptr, *revVol = nullptr,
                       *ecoRitmo = nullptr, *ecoMs = nullptr, *ecoFb = nullptr, *ecoTon = nullptr, *ecoCruz = nullptr, *ecoVol = nullptr;
    std::atomic<int> combinacionPendente { -1 };   // un cambio de programa polo MIDI (a interface cárgaa)
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BXTProcessor)
};
