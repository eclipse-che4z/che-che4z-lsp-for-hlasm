#!/usr/bin/env bash
set -e
brew install ninja llvm@22
echo "LLVM_PATH=$(brew --prefix llvm@22)" >> $GITHUB_ENV
