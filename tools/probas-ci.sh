#!/bin/bash
# As probas de Barullo Xuntos na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows, linux ou sanitizadores, coa compilación en build/
set -eo pipefail
SO="$1"
EXE=""; [ "$SO" = windows ] && EXE=".exe"
CONF=Release; [ "$SO" = sanitizadores ] && CONF=RelWithDebInfo
RT="build/BXTRenderTest_artefacts/$CONF/BXTRenderTest$EXE"

echo "::group::Son (os tres motores, capas, zonas, efectos, combinacións, edición e estado)"
"$RT" proba.wav
echo "::endgroup::"
