#!/usr/bin/env bash
set -e
sudo apt-get update && sudo apt-get install -y ninja-build qemu-user libc6-s390x-cross libstdc++6-s390x-cross
# install cmake v3.29.2
(cd /tmp && curl -sSLo cmake-install.sh https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-linux-x86_64.sh && chmod +x cmake-install.sh && ./cmake-install.sh --skip-license --exclude-subdir --prefix=/usr/local)
echo "PATH=/usr/local/bin:$PATH" >> $GITHUB_ENV
cat scripts/4.0.10.patch | (cd /emsdk/upstream/emscripten; patch -p1)
(NODE_VER=$(node --version) && mkdir /tmp/node-be && cd /tmp/node-be && curl -sSL https://nodejs.org/dist/$NODE_VER/node-$NODE_VER-linux-s390x.tar.xz | tar xJ --strip-components=1)
