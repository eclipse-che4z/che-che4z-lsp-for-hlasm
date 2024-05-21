#!/usr/bin/env sh

set -e

apk update
apk add git curl g++ bison make patch tar xz

mkdir /toolchain

git clone https://github.com/richfelker/musl-cross-make.git

cd musl-cross-make

git checkout -f e149c31c48b4f4a4c9349ddf7bc0027b90245afc

echo "TARGET = aarch64-linux-musl" >> config.mak
echo "OUTPUT = /toolchain/" >> config.mak
echo "BINUTILS_VER = 2.41" >> config.mak
echo "GCC_VER = 14.1.0" >> config.mak
echo "DL_CMD = curl -C - -L -o" >> config.mak
echo "COMMON_CONFIG += CFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\" CXXFLAGS=\"-fdata-sections -ffunction-sections -O2 -g0\"" >> config.mak
echo "BINUTILS_CONFIG = --enable-gprofng=no" >> config.mak

echo "0aec8d432b8473559942a45c12459a5db3a04618 *gcc-14.1.0.tar.xz" > hashes/gcc-14.1.0.tar.xz.sha1
echo "0e008260a958bbd10182ee3384672ae0a310eece *binutils-2.41.tar.xz" > hashes/binutils-2.41.tar.xz.sha1

mkdir patches/gcc-14.1.0
cp patches/gcc-11.2.0/0002-posix_memalign.diff patches/gcc-14.1.0/

make -j 8
make install

cd ..

tar czvf toolchain.tar.gz /toolchain

