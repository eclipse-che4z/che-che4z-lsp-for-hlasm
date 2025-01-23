#!/usr/bin/env bash
set -e
cmake -G Ninja -DCMAKE_VERIFY_HEADER_SETS=On -DCMAKE_BUILD_TYPE=Release -DDISCOVER_TESTS=Off -DCMAKE_C_COMPILER=$(brew --prefix llvm@18)/bin/clang -DCMAKE_CXX_COMPILER=$(brew --prefix llvm@18)/bin/clang++ -DLLVM_PATH=$(brew --prefix llvm@18) -DBUILD_VSIX=Off ../
