#!/usr/bin/env bash
# Builds the WebAssembly engine into web/ and runs the native tests.
# Needs Emscripten on PATH (source ~/emsdk/emsdk_env.sh).
set -euo pipefail
cd "$(dirname "$0")/.."

cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build/native -j >/dev/null
./build/native/tests

emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build/web -j
echo "Built web/engine.js and web/engine.wasm"
