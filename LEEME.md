**Barullo Xuntos** son os sintetizadores de Barullo xuntos nun só instrumento: catro capas e, en cada unha, un dos tres motores da familia, cos seus sons, repartidos polo teclado e mesturados, con reverb e eco comúns.

- **Xuno**: o Juno-106 de [Barullo Xuno](https://github.com/callejodev-dotcom/barullo-xuno), síntese sustractiva (DCO, filtro IR3109, chorus de BBD);
- **7FM**: o DX7 de [Barullo 7FM](https://github.com/callejodev-dotcom/barullo7fm), FM de seis operadores;
- **FM**: o chip de dous operadores do teclado de [Barullo FM](https://github.com/callejodev-dotcom/barullofm), cos seus movementos gravados.

Para que serve: o que mellor fai cada síntese, á vez. O ataque e o brillo da FM (pianos eléctricos, campás, metais con filo) por riba do corpo e o movemento da sustractiva (fondos, cordas, baixos que respiran co filtro). Coma nos teclados dos 80 e 90 que gardaban «combinacións».

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3, CLAP e app | `BarulloXuntos-…-mac.pkg` | `BarulloXuntos-…-mac.zip` |
| **Windows** (64 bits): VST3, CLAP e app | `BarulloXuntos-…-windows-setup.exe` | `BarulloXuntos-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2, CLAP e app | — | `BarulloXuntos-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Que ten

- **Catro capas**. Cada unha:
  - acéndese ou apágase e escolle o seu motor (XUNO, 7FM ou FM);
  - ten o seu son, de calquera banco do seu motor: os 128 de Barullo Xuno, os dous bancos de Barullo 7FM (CLASICOS e BARULLO), os 100 de Barullo FM e os .syx de Juno-106 ou de DX7 que abras (os de fábrica recoñécense polo nome);
  - volume, panorama e envíos á reverb e ao eco;
  - oitava, semitóns, afinación fina (unhas poucas cents entre capas abren o son) e alcance do bend;
  - a súa zona do teclado (desde, ata) e da velocidade (unha capa que só entra tocando forte), e a súa canle MIDI (todas ou unha: para tocar cada capa desde un teclado).
- **O mapa do teclado**: a zona de cada capa na súa cor; arrastra os extremos para partir o teclado. As teclas que soan acéndense.
- **Editar o son de cada capa**, ao momento e sen cortar as notas:
  - Xuno: o panel do Juno-106 (LFO, DCO, HPF, VCF, VCA, ENV, chorus) cos valores en Hz, ms, oitavas e dB, e o portamento e a asignación;
  - 7FM: os seis operadores e o global, coma no DX7, co debuxo do algoritmo;
  - FM: o modulador e a portadora de cada parte, a realimentación e as saídas.
  - «Restaurar» volve ao son do banco.
- **Efectos comúns**: unha reverb (tamaño, brillo, predelay) e un eco (á par do tempo do DAW, de 1/16 a 1/2 con tresillos e puntillos, ou libre en ms; repeticións, ton e cruzado).
- **16 combinacións** de fábrica (Balada 84, Piano e cordas, Cristal na néboa, Baixo e teclas, Metais 1984, Muiñeira, Catro capas…). O cambio de programa do MIDI escolle unha.
- **Presets propios, A/B e desfacer** (do núcleo común): os presets (`.bxtpreset`, en Documentos/Barullo Xuntos/Presets) e o A/B gardan todo, os sons das capas tamén, editados ou non; desfanse os mandos, os sons, as edicións e as combinacións. Cada clic é un paso.
- **ESCOITAR**: mentres está premido, soa un acorde de Do en todo o teclado, para oír as capas e as zonas sen teclado MIDI.
- Un **limitador de seguridade** á saída: nunca pasa de −1 dBFS.

## Como se fixo

- **Os motores son os dos plugins**, sen copialos: o CMake descarga cada un do seu repo nunha versión fixada (Barullo Xuno e Barullo 7FM nun commit fixado, Barullo FM v0.5.0). Se mellora un motor, Barullo Xuntos mellora ao subir esa versión. Cada un soa coma no seu plugin, cos seus mandos de partida (no Juno, a afinación e o bender; no teclado FM, o ecualizador do altofalante).
- **Unha capa** (`Source/motores/Motor.h`) ve calquera motor igual: notas, pedal, bend, roda, o son e as edicións. As notas entran na súa mostra exacta. A afinación fina vai polo mando de afinación do Juno e, no DX7 e no teclado FM, polo bend. No DX7 vai en pasos de 1/64 do alcance do bend: uns 5 cents co alcance de partida (2 semitóns).
- **Os sons** van no formato de cada motor (os 18 bytes do Juno-106, os 155 do DX7, os rexistros do chip). O editor cambia eses bytes e ao motor só lle chegan os que cambian.
- **Os tres soan parecido de forte**: medido cos acordes dos sons de cada banco (`BXTRenderTest --niveis`), o DX7 vai +5 dB e o teclado FM −0,9 dB.
- **A reverb** é unha rede de oito liñas de atraso realimentadas cunha matriz de Householder, con difusión na entrada, un filtro de agudos en cada liña e dúas liñas que se moven moi pouco para que non soe metálica. **O eco**, dous atrasos de ata 2 s con realimentación filtrada e cruzado (`Source/Efectos.h`).

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/motores/Motor.h` | Un motor visto desde unha capa: o son, o editor e os bancos |
| `Source/motores/MotorXuno.cpp` | O Juno-106 de Barullo Xuno |
| `Source/motores/Motor7FM.cpp` | O DX7 de Barullo 7FM |
| `Source/motores/MotorFM.cpp` | O chip do teclado de Barullo FM |
| `Source/Efectos.h` | Reverb e eco |
| `Source/Combinacions.h` | As combinacións de fábrica |
| `Source/Plugin*.cpp`, `Source/Editores.cpp` | O plugin, a interface e o editor dos sons |
| `tools/RenderTest.cpp` | A proba |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.3.0): o texto en galego, o aviso das versións novas, os presets propios, o A/B e o desfacer (`escoita::Estado`; os sons das capas van coma estado propio), o limitador de seguridade (`escoita::Teito`), a CI e os instaladores. O pedal e o reparto das voces son os de cada motor, coma nos seus plugins.

## Probas

- **Os tres motores, con todos os seus sons** (128 + 64 + 100): soan, sen valores non numéricos.
- **Capas**: cada nota á capa da súa zona; a capa de velocidade alta só entra tocando forte; a oitava (o La2 a 220 Hz e o La3 a 440 Hz); cada capa na súa canle MIDI.
- **Bloques**: as catro capas, os tres motores e os efectos, con bloques de 512 e de 37 mostras, dan a mesma saída mostra a mostra (e a súa pegada).
- **Edición**: un cambio no son soa, antes de tocar e coas notas soando; desfeito, o son volve ao do banco bit a bit.
- **Estado, A/B, presets e desfacer**: os parámetros e os sons gárdanse e recupéranse e soan igual.
- **As 16 combinacións** soan.
- **Efectos**: o eco cae no seu tempo; a reverb apágase.
- **Limitador**: catro capas a +6 dB e un acorde forte non pasan de −1 dBFS.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator.

Na CI van en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

A versión sae da liña `project(BarulloXuntos VERSION …)` de `CMakeLists.txt`; o texto da versión, de `packaging/NOTAS-VERSION.md`.

## Créditos e licenzas

- **Barullo Xuntos é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`): podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e coa mesma licenza. Usa JUCE 8 coa súa licenza AGPLv3.
- Os motores e os seus créditos son os de cada plugin: Barullo Xuno, Barullo 7FM e Barullo FM (ver o LEEME de cada un).
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
- Juno-106 é unha marca de Roland Corporation e DX7, de Yamaha Corporation. Barullo Xuntos non ten relación con eles: só le os seus sons e quere soar coma eles.
