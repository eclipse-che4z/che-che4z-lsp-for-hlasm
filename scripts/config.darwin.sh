#!/usr/bin/env bash
set -e
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=$(brew --prefix llvm@15)/bin/clang -DCMAKE_CXX_COMPILER=$(brew --prefix llvm@15)/bin/clang++ -DLLVM_PATH=$(brew --prefix llvm@15) -DBUILD_VSIX=Off -DUSE_PRE_GENERATED_GRAMMAR="generated_parser" ../
