#!/usr/bin/env bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_VSIX=Off -DUSE_PRE_GENERATED_GRAMMAR="generated_parser" ../
