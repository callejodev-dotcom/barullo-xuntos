// Motor.h — Os tres motores de Barullo vistos desde unha capa de Barullo Xuntos.
//
// Cada motor é o do seu plugin, sen tocar (vén do seu repo nunha versión fixada, ver CMakeLists):
//   Xuno  — o Juno-106 de Barullo Xuno (sustractiva: DCO, IR3109, chorus de BBD)
//   7FM   — o DX7 de Barullo 7FM (FM de 6 operadores)
//   FM    — o chip de 2 operadores do teclado de Barullo FM, coas súas «películas»
//
// Un son é un feixe de bytes no formato do seu motor (Son). A interface edítao con Parametros
// (cada un sabe ler e escribir o seu valor nos bytes) e o fío de audio recibe os bytes que
// cambian: así a edición soa ao momento, coma no plugin de cada un.
#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace xuntos {

enum Tipo { XUNO = 0, DX = 1, FM = 2, NUM_TIPOS = 3 };

// un son no formato do seu motor
struct Son
{
    static constexpr int MAX = 160;
    std::array<uint8_t, MAX> d {};
    int n = 0;
    bool operator== (const Son& o) const { return n == o.n && std::equal (d.begin(), d.begin() + n, o.d.begin()); }
    bool operator!= (const Son& o) const { return ! (*this == o); }
};

// un motor dentro dunha capa: todo isto chámase no fío de audio
class Motor
{
public:
    virtual ~Motor() = default;
    virtual void preparar (double frecuencia) = 0;
    virtual void calar() = 0;                          // todo o son fóra, xa
    virtual void soltarTodas() = 0;                    // «all notes off»: as notas acaban coa súa relaxación
    virtual void noteOn (int nota, int velocidade) = 0;
    virtual void noteOff (int nota) = 0;
    virtual void pedal (bool abaixo) = 0;
    virtual void bend (int valor14) = 0;               // a roda de afinación tal como chega (0–16383)
    virtual void roda (int valor7) = 0;                // a roda de modulación (CC1)
    virtual void rangoBend (int semitonos) = 0;
    virtual void afinar (double cents) = 0;            // afinación fina da capa (±50)
    virtual void render (float* L, float* R, int n) = 0;   // escribe (non suma) n mostras
    virtual int vocesActivas() const = 0;
    virtual void setSon (const Son&) = 0;              // un son enteiro
    virtual void editar (int byte, uint8_t valor) = 0; // un byte do son, co son soando
};

// un banco de sons dun motor, cos seus nomes
struct Banco
{
    juce::String nome, titulo;
    std::vector<Son> sons;
    juce::StringArray nomes, descricions;
};

// un parámetro do editor: nome, rango, como se ve e onde vive nos bytes do son
struct Parametro
{
    enum Vista { NUMERO, DESLIZADOR, TECLAS };
    juce::String nome, grupo, axuda;
    int min = 0, max = 127;
    Vista vista = NUMERO;
    juce::StringArray opcions;                         // TECLAS: o texto de cada valor
    std::function<juce::String (int)> texto;           // opcional: o valor con unidades
    std::function<int (const Son&)> ler;
    std::function<void (Son&, int)> escribir;
};

// o que se sabe de cada motor fóra do fío de audio
struct Fabrica
{
    const char* nome;                                  // «Xuno», «7FM», «FM»
    const char* descricion;
    std::unique_ptr<Motor> (*crear)();
    std::vector<Banco> (*bancos)();                    // os sons que van dentro
    juce::String (*nomeDo) (const Son&);               // o nome dun son, se se sabe
    // un .syx do formato do motor (Xuno: Juno-106; 7FM: DX7). nullptr: o motor non le ficheiros
    bool (*lerSyx) (const juce::MemoryBlock&, Banco&, juce::String& erro);
    std::vector<Parametro> (*parametros)();
    // un debuxo do son para o editor (7FM: o algoritmo). nullptr: sen debuxo
    void (*pintar) (juce::Graphics&, juce::Rectangle<float>, const Son&, juce::Colour tinta, juce::Colour acento);
    double ganancia;                                   // para que os tres soen parecido de forte
};

const Fabrica& fabrica (int tipo);

// as tres, cada unha no seu ficheiro (MotorXuno.cpp, Motor7FM.cpp, MotorFM.cpp)
const Fabrica& fabricaXuno();
const Fabrica& fabrica7FM();
const Fabrica& fabricaFM();

inline const Fabrica& fabrica (int tipo)
{
    return tipo == DX ? fabrica7FM() : tipo == FM ? fabricaFM() : fabricaXuno();
}

} // namespace xuntos
