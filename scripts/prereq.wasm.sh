#!/usr/bin/env bash
set -e
sudo apt-get update && sudo apt-get install -y ninja-build
# install cmake v3.29.2
(cd /tmp && curl -sSLo cmake-install.sh https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-linux-x86_64.sh && chmod +x cmake-install.sh && ./cmake-install.sh --skip-license --exclude-subdir --prefix=/usr/local)
echo "PATH=/usr/local/bin:$PATH" >> $GITHUB_ENV
cat scripts/4.0.10.patch | (cd /emsdk/upstream/emscripten; patch -p1)
