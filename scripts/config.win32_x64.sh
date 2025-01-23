#!/usr/bin/env bash
set -e
cmake -DCMAKE_VERIFY_HEADER_SETS=On -DCMAKE_BUILD_TYPE=Release -DDISCOVER_TESTS=Off -DBUILD_VSIX=Off ../
