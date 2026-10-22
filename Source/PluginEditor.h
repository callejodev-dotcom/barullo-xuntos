#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
    // A familia Barullo: o azul mariño da noite de Vigo, a crema da letra de Escoita e a luz laranxa,
    // nun aparello de estudio (paneis en relevo, pantallas de cristal, visores dourados). Cada capa
    // leva a súa cor, a mesma no mapa do teclado.
    namespace col
    {
        const juce::Colour noite    { 0xff0c111f };
        const juce::Colour crema    { 0xfffff7c1 };
        const juce::Colour crema2   { 0xffbdb792 };
        const juce::Colour crema3   { 0xff857f67 };
        const juce::Colour laranxa  { 0xffe08b16 };
        const juce::Colour ouro     { 0xffedba23 };
        const juce::Colour verde    { 0xff9dc6a0 };
        const juce::Colour azul     { 0xff8fb0e8 };
        const juce::Colour panel1   { 0xff1c2540 }, panel2 { 0xff111830 };
        const juce::Colour tecla1   { 0xff27314f }, tecla2 { 0xff1a2239 };
        const juce::Colour visor    { 0xff070b16 };
        const juce::Colour cristal1 { 0xff142046 }, cristal2 { 0xff070c1c };
        const juce::Colour lina     { 0xff2a3350 };
        inline juce::Colour capa (int k) { static const juce::Colour c[4] = { laranxa, verde, ouro, azul }; return c[k & 3]; }
    }

    juce::Font body (float size);    // Inter
    juce::Font semi (float size);    // Montserrat SemiBold
    juce::Font cond (float size);    // Montserrat Bold
    juce::Font caps (float size, float tracking = 0.18f);
    juce::String upper (const juce::String&);
    juce::String noteName (int midi);
    void drawCaps (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, float tracking = 0.18f);
    void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour);
    void paintPanel (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintWell (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintGlass (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintKey (juce::Graphics&, juce::Rectangle<float>, bool over, bool down);
    void paintLed (juce::Graphics&, juce::Point<float>, bool on, juce::Colour = col::laranxa);
    void groupTitle (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour = col::laranxa);

    struct Look : juce::LookAndFeel_V4
    {
        Look();
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        juce::Font getPopupMenuFont() override { return body (15.0f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    };

    // mando co anel de LED e o seu valor nun visor debaixo
    struct Knob : juce::Component
    {
        Knob();
        void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (14)); }
        void paint (juce::Graphics&) override;
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    // un deslizador vertical co seu visor (o panel do Juno no editor)
    struct Fader : juce::Component
    {
        Fader();
        void resized() override;
        void paint (juce::Graphics&) override;
        juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
        juce::String name;
        static constexpr int readoutH = 18, labelH = 14;
    };

    // un número nun visor: arrastrar cara arriba ou abaixo (ou a roda do rato) cámbiao
    struct NumBox : juce::Component, juce::SettableTooltipClient
    {
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void setValue (int v, bool notify);
        int value = 0, min = 0, max = 127;
        juce::String name;                        // rótulo pequeno enriba (opcional)
        juce::Colour tint = col::ouro;
        std::function<juce::String (int)> text;
        std::function<void (int)> onChange;
    private:
        int startValue = 0;
        float wheelAcc = 0;
    };

    class MainView;

    // o mapa do teclado: a zona de notas de cada capa (arrastra os extremos) e as teclas que soan
    class ZoneMap : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit ZoneMap (BXTProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
    private:
        juce::Rectangle<float> keys() const;
        float xOf (int note) const;
        int noteAt (float x) const;
        BXTProcessor& proc;
        int dragLayer = -1;
        bool dragHigh = false;
    };

    // unha capa: o motor, o son, a mestura, a afinación e a zona
    class LayerStrip : public juce::Component
    {
    public:
        LayerStrip (BXTProcessor&, MainView&, int index);
        void paint (juce::Graphics&) override;
        void resized() override;
        void refresh();
        void tick();
    private:
        void soundsMenu();
        BXTProcessor& proc;
        MainView& main;
        const int k;
        juce::TextButton onBtn, engineBtn[3], prevBtn { U8 ("‹") }, nextBtn { U8 ("›") }, soundsBtn { "Sons" }, editBtn { "Editar" }, chanBtn;
        Knob volume, pan, rev, eco, oct, transp, cents, bend;
        NumBox noteLo, noteHi, velLo, velHi;
        std::unique_ptr<juce::ParameterAttachment> onAtt, engineAtt, chanAtt, noteLoAtt, noteHiAtt, velLoAtt, velHiAtt;
        juce::Rectangle<int> soundBox;
        float level = 0;
        int voices = 0;
        std::unique_ptr<juce::FileChooser> chooser;
        static juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }
    };

    // o editor do son dunha capa: os parámetros do seu motor (o panel do Juno, os operadores do
    // DX7 ou os do chip FM), que soan ao momento
    class SoundEditor : public juce::Component
    {
    public:
        SoundEditor (BXTProcessor&, MainView&);
        void open (int layer);
        int layer() const { return k; }
        void paint (juce::Graphics&) override;
        void resized() override;
        void refresh();   // os valores desde o son (despois de desfacer, restaurar…)
    private:
        struct Control
        {
            xuntos::Parametro p;
            std::unique_ptr<Fader> fader;
            std::unique_ptr<NumBox> num;
            juce::OwnedArray<juce::TextButton> keys;
            juce::Rectangle<int> area;   // as teclas e o seu rótulo
        };
        void build();
        void set (Control&, int value);
        int labelW() const { return tipo == xuntos::DX ? 64 : 176; }   // o rótulo das filas da táboa
        BXTProcessor& proc;
        MainView& main;
        int k = 0, tipo = 0;
        bool panelMode = false;   // Xuno: o panel; 7FM e FM: unha táboa por grupos
        std::vector<std::unique_ptr<Control>> controls;
        juce::StringArray groups;
        std::vector<juce::Rectangle<int>> groupAreas;
        juce::Rectangle<int> drawing;
        juce::TextButton restoreBtn { "Restaurar" }, closeBtn { "Pechar" };
    };

    class MainView : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int W = 1300, H = 800;
        explicit MainView (BXTProcessor&);
        ~MainView() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void timerCallback() override;
        void mouseDown (const juce::MouseEvent&) override;   // (global) cada clic, un paso de desfacer
        void openEditor (int layer);
        void closeEditor();
        bool editorOpen() const { return editor.isVisible(); }
        void showMessage (const juce::String& m) { message = m; messageTime = juce::Time::getMillisecondCounter(); repaint (screenArea); }
        void refreshAll();
        // de onde é o son: «7FM · BARULLO», «XUNO» (o banco, se non se chama coma o motor)
        static juce::String origin (BXTProcessor& p, int layer, int tipo)
        {
            const juce::String m = xuntos::fabrica (tipo).nome, b = p.bancoSon (layer, tipo);
            return b.equalsIgnoreCase (m) || b.isEmpty() ? m : m + juce::String::fromUTF8 (" · ") + b;
        }

    private:
        void combinationsMenu();
        void functionMenu();
        void showPresetsMenu();
        void refreshState();

        BXTProcessor& proc;
        Look look;
        juce::TooltipWindow tooltips { this, 500 };

        juce::TextButton combBtn, prevComb, nextComb, presetsBtn { "Presets" }, abBtn[2], undoBtn, redoBtn, funcBtn;
        juce::TextButton listenBtn { "Escoitar" }, ritmoBtn, cruzBtn { "Cruzado" };
        std::unique_ptr<juce::ParameterAttachment> ritmoAtt, cruzAtt;
        Knob revSize, revBright, revPre, revVol, ecoTime, ecoFb, ecoTone, ecoVol, volume;
        juce::OwnedArray<LayerStrip> strips;
        ZoneMap zones;
        SoundEditor editor;
        juce::TextButton newVersion;
        juce::Rectangle<int> headerArea, screenArea, layersArea, fxArea;
        juce::String message;
        juce::uint32 messageTime = 0;
        uint32_t seenChanges = 0;
        bool listening = false;
        juce::SharedResourcePointer<escoita::Actualizacions> versions;
    };
}

class BXTEditor : public juce::AudioProcessorEditor
{
public:
    explicit BXTEditor (BXTProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::noite); }
    ui::MainView& view() { return main; }
private:
    BXTProcessor& proc;
    ui::MainView main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BXTEditor)
};
