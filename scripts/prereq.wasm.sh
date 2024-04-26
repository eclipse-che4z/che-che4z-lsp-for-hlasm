#!/usr/bin/env bash
set -e
sudo apt-get update && sudo apt-get install -y ninja-build
# patch failing emscripten code
sed -i '/^#if EXPORT_ES6 && USE_ES6_IMPORT_META && !SINGLE_FILE/{n;s/} else {/} else if(!Module["wasmModule"]) {/}' /emsdk/upstream/emscripten/src/preamble.js
# install cmake v3.29.2
(cd /tmp && curl -sSLo cmake-install.sh https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-linux-x86_64.sh && chmod +x cmake-install.sh && ./cmake-install.sh --skip-license --exclude-subdir --prefix=/usr/local)
