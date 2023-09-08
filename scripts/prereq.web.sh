#!/usr/bin/env bash
sudo apt-get update && sudo apt-get install -y ninja-build
# patch failing emscripten code
sed -i '/^#if EXPORT_ES6 && USE_ES6_IMPORT_META && !SINGLE_FILE/{n;s/} else {/} else if(!Module["wasmModule"]) {/}' /emsdk/upstream/emscripten/src/preamble.js
