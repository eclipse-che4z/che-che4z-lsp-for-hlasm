#!/usr/bin/env sh

set -e

apk update
apk add git curl g++ bison make patch tar xz gawk

git clone -n --filter=tree:0 https://gitlab.alpinelinux.org/alpine/aports.git /aports
(cd /aports; git sparse-checkout set --no-cone main/gcc ; git checkout 1a03a2a9c9e77f1a07d48b6f6805e72c8c63c03f)

mkdir /toolchain

git clone https://github.com/richfelker/musl-cross-make.git

cd musl-cross-make

git checkout -f e149c31c48b4f4a4c9349ddf7bc0027b90245afc

echo "TARGET = $1" >> config.mak
echo "OUTPUT = /toolchain/" >> config.mak
echo "BINUTILS_VER = 2.41" >> config.mak
echo "GCC_VER = 14.2.0" >> config.mak
echo "DL_CMD = curl --retry 20 --retry-max-time 120 -C - -L -o" >> config.mak
echo "COMMON_CONFIG += CFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\" CXXFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\"" >> config.mak
echo "BINUTILS_CONFIG = --enable-gprofng=no" >> config.mak

echo "d91ecc3d20ce6298bd95f9b09cc51dc6d3c73ae3 *gcc-14.2.0.tar.xz" > hashes/gcc-14.2.0.tar.xz.sha1
echo "0e008260a958bbd10182ee3384672ae0a310eece *binutils-2.41.tar.xz" > hashes/binutils-2.41.tar.xz.sha1

mkdir patches/gcc-14.2.0
cp /aports/main/gcc/*.patch patches/gcc-14.2.0/
cp ../scripts/gcc-0017.patch patches/gcc-14.2.0/0017-Alpine-musl-package-provides-libssp_nonshared.a.-We-.patch

make -j 8
make install

cd ..

tar czvf toolchain.tar.gz /toolchain
