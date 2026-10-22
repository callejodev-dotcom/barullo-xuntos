// Efectos.h — Os dous efectos comúns de Barullo Xuntos, aos que cada capa manda o seu envío:
//
//  Reverb: unha rede de oito liñas de atraso realimentadas (FDN) cunha matriz de Householder,
//          precedida dun predelay e de catro filtros paso todo que difunden o ataque. Cada liña
//          leva o seu filtro de agudos (o «brillo») e a súa ganancia, calculada para que todas
//          caian 60 dB no mesmo tempo (o «tamaño»); dúas liñas móvense un chisco para que non soe
//          metálica.
//  Eco:    dous atrasos (esquerda e dereita) de ata 2 s, en ms ou á par do tempo do DAW, con
//          realimentación, un filtro de agudos nela e, se se quere, cruzado (pingue-pongue).
//
// Sen JUCE e sen reservar memoria no fío de audio (todo en preparar()).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace xuntos {

// unha liña de atraso con lectura fraccionaria
struct Atraso
{
    std::vector<float> b;
    int w = 0;
    void preparar (int maxMostras) { b.assign ((size_t) std::max (4, maxMostras + 2), 0.0f); w = 0; }
    void limpar() { std::fill (b.begin(), b.end(), 0.0f); }
    void escribir (float x) { b[(size_t) w] = x; if (++w >= (int) b.size()) w = 0; }
    float ler (double atras) const   // atras en mostras (≥ 1)
    {
        const int n = (int) b.size();
        atras = std::clamp (atras, 1.0, (double) (n - 2));
        const int i = (int) atras;
        const float f = (float) (atras - i);
        int a = w - i; if (a < 0) a += n;
        int c = a - 1; if (c < 0) c += n;
        return b[(size_t) a] + (b[(size_t) c] - b[(size_t) a]) * f;
    }
};

class Reverb
{
public:
    // tamaño 0–1 (T60 de 0,3 a 10 s), brillo 0–1 (corte de 1,5 a 16 kHz), predelay en ms
    void axustar (double tamano, double brillo, double predelayMs)
    {
        t60 = 0.3 * std::pow (10.0 / 0.3, std::clamp (tamano, 0.0, 1.0));
        corte = 1500.0 * std::pow (16000.0 / 1500.0, std::clamp (brillo, 0.0, 1.0));
        pre = std::clamp (predelayMs, 0.0, 200.0) * fs / 1000.0;
        escala = 0.55 + 0.9 * std::clamp (tamano, 0.0, 1.0);   // salas máis grandes, liñas máis longas
        calcular();
    }

    void preparar (double frecuencia)
    {
        fs = frecuencia;
        predelay[0].preparar ((int) (0.21 * fs)); predelay[1].preparar ((int) (0.21 * fs));
        for (size_t i = 0; i < difusores.size(); ++i) for (int c = 0; c < 2; ++c) difusores[i][(size_t) c].preparar ((int) (0.02 * fs));
        for (auto& l : linas) l.preparar ((int) (0.2 * fs));
        calcular();
        limpar();
    }

    void limpar()
    {
        for (auto& p : predelay) p.limpar();
        for (auto& d : difusores) for (auto& c : d) c.limpar();
        for (auto& l : linas) l.limpar();
        filtro.fill (0.0); fase = 0;
    }

    // a reverb dun envío estéreo (só o molleiro): l e r entran e saen
    void procesar (float& l, float& r)
    {
        predelay[0].escribir (l); predelay[1].escribir (r);
        double x[2] = { predelay[0].ler (std::max (1.0, pre)), predelay[1].ler (std::max (1.0, pre)) };
        for (size_t i = 0; i < difusores.size(); ++i)
            for (int c = 0; c < 2; ++c)
            {
                auto& d = difusores[i][(size_t) c];
                const double atras = DIFUSION_MS[i] * (c ? 1.07 : 1.0) * fs / 1000.0;
                const double v = d.ler (atras), u = x[c] + 0.62 * v;
                d.escribir ((float) u);
                x[c] = v - 0.62 * u;
            }

        // as oito liñas, co seu filtro e a súa ganancia; dúas móvense moi pouco
        fase += 2.0 * PI * 0.37 / fs;
        if (fase > 2.0 * PI) fase -= 2.0 * PI;
        std::array<double, N> s {};
        double suma = 0;
        for (int i = 0; i < N; ++i)
        {
            double atras = lonx[(size_t) i];
            if (i == 2) atras += 0.0007 * fs * std::sin (fase);
            if (i == 5) atras += 0.0007 * fs * std::cos (fase * 1.3);
            double v = linas[(size_t) i].ler (atras);
            filtro[(size_t) i] += (v - filtro[(size_t) i]) * coefFiltro;
            v = filtro[(size_t) i] * ganancia[(size_t) i];
            s[(size_t) i] = v; suma += v;
        }
        // Householder: s − (2/N)·Σs; as entradas alternan esquerda e dereita
        const double k = 2.0 / N * suma;
        double sl = 0, sr = 0;
        for (int i = 0; i < N; ++i)
        {
            const double entrada = (i & 1) ? x[1] : x[0];
            linas[(size_t) i].escribir ((float) (s[(size_t) i] - k + entrada * 0.35));
            if (i & 1) sr += s[(size_t) i] * ((i & 2) ? -1.0 : 1.0); else sl += s[(size_t) i] * ((i & 4) ? -1.0 : 1.0);
        }
        l = (float) (sl * 2.0); r = (float) (sr * 2.0);   // co ruído continuo, a cola sae uns 3 dB por debaixo do que entra (tamaño 0,45)
    }

private:
    static constexpr int N = 8;
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double LINAS_MS[N] = { 29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.7, 73.1 };
    static constexpr double DIFUSION_MS[4] = { 4.77, 3.59, 2.73, 1.89 };

    void calcular()
    {
        for (int i = 0; i < N; ++i)
        {
            lonx[(size_t) i] = LINAS_MS[i] * escala * fs / 1000.0;
            ganancia[(size_t) i] = std::pow (10.0, -3.0 * lonx[(size_t) i] / (t60 * fs));   // −60 dB en t60
        }
        coefFiltro = 1.0 - std::exp (-2.0 * PI * corte / fs);
    }

    double fs = 48000, t60 = 2.0, corte = 6000, pre = 0, escala = 1.0, coefFiltro = 0.5, fase = 0;
    std::array<Atraso, 2> predelay;
    std::array<std::array<Atraso, 2>, 4> difusores;
    std::array<Atraso, N> linas;
    std::array<double, N> lonx {}, ganancia {}, filtro {};
};

class Eco
{
public:
    // tempo en ms, realimentación 0–0,95, ton 0–1 (corte de 1 a 16 kHz), cruzado: pingue-pongue
    void axustar (double ms, double realimentacion, double ton, bool cruzadoOn)
    {
        obxectivo = std::clamp (ms, 10.0, 2000.0) * fs / 1000.0;
        fb = std::clamp (realimentacion, 0.0, 0.95);
        coef = 1.0 - std::exp (-2.0 * 3.14159265358979323846 * 1000.0 * std::pow (16.0, std::clamp (ton, 0.0, 1.0)) / fs);
        cruzado = cruzadoOn;
        if (primeiro) { tempo = obxectivo; primeiro = false; }   // o primeiro tempo, xa (sen esvarar desde o anterior)
    }

    void preparar (double frecuencia)
    {
        fs = frecuencia;
        for (auto& a : atraso) a.preparar ((int) (2.05 * fs));
        limpar();
        tempo = obxectivo;
        primeiro = true;
    }

    void limpar() { for (auto& a : atraso) a.limpar(); lp[0] = lp[1] = 0; }

    void procesar (float& l, float& r)
    {
        tempo += (obxectivo - tempo) * 0.0004;   // un cambio de tempo esvara (coma unha cinta), sen saltos
        double a = atraso[0].ler (tempo), b = atraso[1].ler (tempo);
        lp[0] += (a - lp[0]) * coef; lp[1] += (b - lp[1]) * coef;
        if (cruzado)
        {
            atraso[0].escribir ((float) (0.5 * (l + r) + lp[1] * fb));   // a entrada vai á esquerda e despois cruza
            atraso[1].escribir ((float) (lp[0] * fb));
        }
        else
        {
            atraso[0].escribir ((float) (l + lp[0] * fb));
            atraso[1].escribir ((float) (r + lp[1] * fb));
        }
        l = (float) a; r = (float) b;
    }

private:
    double fs = 48000, obxectivo = 24000, tempo = 24000, fb = 0.35, coef = 0.5;
    bool cruzado = false, primeiro = true;
    std::array<Atraso, 2> atraso;
    double lp[2] {};
};

} // namespace xuntos
