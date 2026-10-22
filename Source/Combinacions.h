// Combinacions.h — As combinacións que veñen co plugin: capas de motores distintos xa mesturadas
// e repartidas polo teclado. Cada son dise polo seu nome no banco do seu motor (Xuno: «A21»; 7FM:
// banco e número; FM: o nome), así non depende da orde.
#pragma once
#include "motores/Motor.h"

namespace xuntos {

struct CapaDeFabrica
{
    int tipo = -1;              // −1: capa apagada; XUNO, DX ou FM
    const char* banco = "";     // o nome do banco no seu motor («XUNO», «CLASICOS», «BARULLO», «FM»)
    const char* son = "";       // o comezo do nome do son no banco («A21», «01», «Gaita»)
    float volume = 0;           // dB
    float pan = 0;              // −1 … +1
    int oitava = 0, semitons = 0;
    float cents = 0;
    int notaBaixa = 0, notaAlta = 127, velBaixa = 1, velAlta = 127;
    float reverb = 0.2f, eco = 0;
};

struct Combinacion
{
    const char* nome;
    const char* descricion;
    CapaDeFabrica capas[4];
    float revTamano = 0.45f, revBrillo = 0.55f, revPredelay = 20, revVolume = 0.8f;
    int ecoRitmo = 4;           // 0 libre, 1 1/16, 2 1/8T, 3 1/8, 4 1/8., 5 1/4T, 6 1/4, 7 1/4., 8 1/2
    float ecoMs = 350, ecoRealimentacion = 0.35f, ecoTon = 0.5f;
    bool ecoCruzado = true;
    float ecoVolume = 0.6f;
};

inline const Combinacion COMBINACIONS[] = {
    { "Balada 84", "O piano eléctrico do DX7 e debaixo o fondo morno do Juno, que entra amodo: a balada dos 80.",
      { { DX, "BARULLO", "01", -2, -0.1f, 0, 0, 0, 0, 127, 1, 127, 0.25f, 0.08f },
        { XUNO, "XUNO", "A71", -11, 0.1f, 0, 0, 0, 0, 127, 1, 127, 0.35f, 0 } },
      0.5f, 0.5f, 25, 0.8f, 6, 380, 0.3f, 0.45f, true, 0.5f },
    { "Piano e cordas", "Piano acústico de FM e as cordas do Juno co chorus I: cada un no seu sitio do panorama.",
      { { DX, "CLASICOS", "01", 3.5, -0.25f, 0, 0, 0, 0, 127, 1, 127, 0.25f, 0 },
        { XUNO, "XUNO", "A21", -4.5, 0.3f, 0, 0, 0, 0, 127, 1, 127, 0.4f, 0 } },
      0.55f, 0.5f, 20, 0.8f },
    { "Cristal na néboa", "As campás do DX7 por riba e a néboa do Juno, que aparece e respira por debaixo.",
      { { DX, "BARULLO", "26", 0.5, 0, 0, 0, 0, 0, 127, 1, 127, 0.45f, 0.2f },
        { XUNO, "XUNO", "B41", -3.5, 0, 0, 0, 0, 0, 127, 1, 127, 0.5f, 0 } },
      0.75f, 0.6f, 30, 0.9f, 6, 420, 0.45f, 0.4f, true, 0.6f },
    { "Baixo e teclas", "Partido: o baixo groso do Juno na man esquerda e o piano de cristal do DX7 na dereita (desde o Do3).",
      { { XUNO, "XUNO", "A55", -2, 0, 0, 0, 0, 0, 59, 1, 127, 0.05f, 0 },
        { DX, "BARULLO", "02", 0, 0, 0, 0, 0, 60, 127, 1, 127, 0.3f, 0.1f } },
      0.45f, 0.55f, 15, 0.7f },
    { "Metais 1984", "Os metais do Juno co filtro que abre e os do DX7 co seu filo: o «bwaaah» e o brillo á vez.",
      { { XUNO, "XUNO", "A11", 0, -0.2f, 0, 0, 0, 0, 127, 1, 127, 0.2f, 0 },
        { DX, "BARULLO", "09", -4, 0.2f, 0, 0, 0, 0, 127, 1, 127, 0.2f, 0 } },
      0.45f, 0.5f, 15, 0.7f },
    { "Órgano e coro", "O órgano de jazz do DX7 e o coro sen palabras do Juno, moi baixiño.",
      { { DX, "CLASICOS", "08", 0, 0, 0, 0, 0, 0, 127, 1, 127, 0.25f, 0 },
        { XUNO, "XUNO", "A73", -11, 0, 0, 0, 0, 0, 127, 1, 127, 0.45f, 0 } },
      0.6f, 0.45f, 20, 0.8f },
    { "Toca forte", "Por velocidade: tocando suave, o solista do Juno; forte, entran as campás tubulares do DX7.",
      { { XUNO, "XUNO", "B23", 0, 0, 0, 0, 0, 0, 127, 1, 127, 0.25f, 0.15f },
        { DX, "CLASICOS", "27", -3, 0, 0, 0, 0, 0, 127, 100, 127, 0.4f, 0.2f } },
      0.5f, 0.55f, 20, 0.8f, 4, 350, 0.35f, 0.5f, true, 0.5f },
    { "Muiñeira", "A gaita do teclado FM, co seu movemento gravado, e un ronco do Juno dúas oitavas por debaixo.",
      { { FM, "FM", "Gaita", 2.5, 0, 0, 0, 0, 0, 127, 1, 127, 0.3f, 0 },
        { XUNO, "XUNO", "B57", -9.5, 0, -1, 0, 0, 0, 127, 1, 127, 0.2f, 0 } },
      0.5f, 0.5f, 20, 0.7f },
    { "Cordas de tres", "As cordas dos tres motores xuntas: o teclado FM, o DX7 e o Juno, abertas no panorama.",
      { { XUNO, "XUNO", "A23", -1, 0, 0, 0, 0, 0, 127, 1, 127, 0.4f, 0 },
        { DX, "BARULLO", "15", -4, -0.6f, 0, 0, 0, 0, 127, 1, 127, 0.4f, 0 },
        { FM, "FM", "Cordas", -4, 0.6f, 0, 0, 6, 0, 127, 1, 127, 0.4f, 0 } },
      0.65f, 0.5f, 25, 0.85f },
    { "Slap e fondo", "Partido: o baixo slap do DX7 abaixo e o fondo PWM do Juno arriba.",
      { { DX, "BARULLO", "22", 5, 0, 0, 0, 0, 0, 54, 1, 127, 0.05f, 0 },
        { XUNO, "XUNO", "A72", 1, 0, 0, 0, 0, 55, 127, 1, 127, 0.35f, 0.1f } },
      0.5f, 0.5f, 15, 0.7f, 6, 400, 0.3f, 0.4f, true, 0.4f },
    { "House 92", "Partido, todo Juno: o baixo de house na esquerda e o acorde das pistas na dereita, co eco a corchea con punto.",
      { { XUNO, "XUNO", "B14", 1.5, 0, 0, 0, 0, 0, 52, 1, 127, 0, 0 },
        { XUNO, "XUNO", "B11", -0.5, 0, 0, 0, 0, 53, 127, 1, 127, 0.2f, 0.35f } },
      0.4f, 0.6f, 10, 0.6f, 4, 340, 0.4f, 0.55f, true, 0.6f },
    { "Synthpop", "O solista synthpop do Juno e o solista do DX7 unha oitava por riba, moi baixo: o brillo dos 80.",
      { { XUNO, "XUNO", "B23", 0, 0, 0, 0, 0, 0, 127, 1, 127, 0.2f, 0.2f },
        { DX, "BARULLO", "29", -10, 0, 1, 0, 0, 0, 127, 1, 127, 0.2f, 0.2f } },
      0.45f, 0.6f, 20, 0.7f, 4, 360, 0.3f, 0.5f, true, 0.5f },
    { "Marimba no espazo", "A marimba do DX7 seca e o espazo do Juno detrás, co eco cruzado.",
      { { DX, "CLASICOS", "26", 4, 0, 0, 0, 0, 0, 127, 1, 127, 0.2f, 0.3f },
        { XUNO, "XUNO", "A78", -6, 0, 0, 0, 0, 0, 127, 1, 127, 0.6f, 0 } },
      0.7f, 0.55f, 30, 0.8f, 6, 450, 0.4f, 0.45f, true, 0.6f },
    { "Catro capas", "Todo xunto: o baixo sub do Juno abaixo e, arriba, as cordas do Juno, o piano eléctrico do DX7 e o coro do teclado FM.",
      { { XUNO, "XUNO", "A57", -2, 0, 0, 0, 0, 0, 47, 1, 127, 0, 0 },
        { XUNO, "XUNO", "A21", -9, -0.4f, 0, 0, 0, 48, 127, 1, 127, 0.4f, 0 },
        { DX, "BARULLO", "01", -2, 0, 0, 0, 0, 48, 127, 1, 127, 0.25f, 0.05f },
        { FM, "FM", "Coro feminino", -12, 0.4f, 0, 0, 0, 48, 127, 1, 127, 0.4f, 0 } },
      0.6f, 0.5f, 25, 0.85f },
    { "Mar de Vigo", "O mar do Juno e a celesta do DX7, co eco cruzado lento.",
      { { XUNO, "XUNO", "B42", -2, 0, 0, 0, 0, 0, 127, 1, 127, 0.5f, 0 },
        { DX, "BARULLO", "27", 1, 0, 0, 0, 0, 0, 127, 1, 127, 0.4f, 0.35f } },
      0.8f, 0.5f, 40, 0.9f, 7, 520, 0.5f, 0.35f, true, 0.6f },
    { "Inicial", "Unha capa só: o son de partida do Juno, para empezar unha combinación desde cero.",
      { { XUNO, "XUNO", "B88", 0, 0, 0, 0, 0, 0, 127, 1, 127, 0.2f, 0 } },
      0.45f, 0.55f, 20, 0.8f },
};

constexpr int NUM_COMBINACIONS = (int) (sizeof (COMBINACIONS) / sizeof (COMBINACIONS[0]));

} // namespace xuntos
