#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the extracted ps5-payload-dev SDK v0.42 directory}"
mkdir -p dist
"$PS5_PAYLOAD_SDK/bin/prospero-clang" -D__PROSPERO__=1 -std=gnu11 -O2 -Wall -Wextra \
  -Isrc src/main.c src/probe.c src/transport_cam.c \
  -L"$PS5_PAYLOAD_SDK/target/lib" -o dist/PS5DriveDiagnostic.elf
