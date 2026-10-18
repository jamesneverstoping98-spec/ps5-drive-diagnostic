#!/usr/bin/env bash
# Uses the verified upstream native application packaging toolchain.
set -euo pipefail
kit=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${SWANSTATION_SOURCE:?Set SWANSTATION_SOURCE to a dedicated upstream checkout}"
upstream=$(cd -- "$SWANSTATION_SOURCE" && pwd)
expected=b995783c93b1e28560a23d67335e5ee2e257e2db
[[ $(git -C "$upstream" rev-parse HEAD) == "$expected" ]] || { echo "Wrong upstream revision" >&2; exit 2; }
[[ ! -e $upstream/drive-diagnostic ]] || { echo "drive-diagnostic already exists; use a fresh checkout" >&2; exit 2; }
mkdir -p "$upstream/drive-diagnostic/src" "$upstream/drive-diagnostic/sce_sys"
cp "$kit"/src/* "$upstream/drive-diagnostic/src/"
cp "$upstream"/sce_sys/*.png "$upstream/drive-diagnostic/sce_sys/"
python3 - "$upstream" <<'PY'
import json,sys
from pathlib import Path
p=Path(sys.argv[1]); meta=json.loads((p/'sce_sys/param.json').read_text())
meta.update(titleId='PPSA98519',conceptId='98519',contentId='UP9000-PPSA98519_00-DRIVEDIAG0000001',contentVersion='01.000.000')
# This identifier is a local homebrew placeholder, not a registered title.
meta['localizedParameters']={'defaultLanguage':'en-US','en-US':{'titleName':'PS5 Drive Diagnostic'}}
(p/'drive-diagnostic/sce_sys/param.json').write_text(json.dumps(meta,indent=2))
PY
cd "$upstream"
# Call build.sh directly to avoid SwanStation's emulator make prerequisites.
APP_SOURCE_DIR=drive-diagnostic/src APP_PARAM=drive-diagnostic/sce_sys/param.json \
APP_SCE_SYS=drive-diagnostic/sce_sys APP_ASSETS= APP_VULKAN=0 \
APP_LAPY_HELPER=0 APP_DEFINITIONS=__PROSPERO__=1 APP_INCLUDE_PATHS= APP_STATIC_ARCHIVES= \
APP_WRAP_SYMBOLS= APP_RUNTIME_MODULES= APP_ROOT_FILES= \
bash tools/build.sh "${1:-folder}"
echo 'Result: upstream dist/PPSA98519 (or selected packaging format). Build recipe is not console validated.'
