#!/usr/bin/env sh

set -e

apk update
apk add git curl g++ bison make patch tar xz gawk

mkdir /toolchain

git clone https://github.com/richfelker/musl-cross-make.git

cd musl-cross-make

git checkout -f 6f3701d08137496d5aac479e3a3977b5ae993c1f

# pre-download closest one
mkdir -p sources && curl --retry 20 --retry-max-time 120 -C - -L -o sources/config.sub https://raw.githubusercontent.com/gcc-mirror/gcc/fe38e08bfe1758c56344a515a7d73105c9f937cc/config.sub

echo "TARGET = $1" >> config.mak
echo "OUTPUT = /toolchain/" >> config.mak
echo "BINUTILS_VER = 2.41" >> config.mak
echo "GCC_VER = 14.2.0" >> config.mak
echo "DL_CMD = curl --retry 20 --retry-max-time 120 -C - -L -o" >> config.mak
echo "COMMON_CONFIG += CFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\" CXXFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\"" >> config.mak
echo "BINUTILS_CONFIG = --enable-gprofng=no" >> config.mak
echo "GNU_SITE = https://mirrors.ocf.berkeley.edu/gnu" >> config.mak

echo "0e008260a958bbd10182ee3384672ae0a310eece *binutils-2.41.tar.xz" > hashes/binutils-2.41.tar.xz.sha1

make -j 8
make install

cd ..

tar czvf toolchain.tar.gz /toolchain
