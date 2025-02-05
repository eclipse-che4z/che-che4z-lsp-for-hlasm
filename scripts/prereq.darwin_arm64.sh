#!/usr/bin/env bash
set -e
brew install ninja llvm@18
echo "LLVM_PATH=$(brew --prefix llvm@18)" >> $GITHUB_ENV
