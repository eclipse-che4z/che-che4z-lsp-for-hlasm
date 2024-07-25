#!/usr/bin/env bash
set -e
node --experimental-wasm-eh $1.js
node --experimental-wasm-eh --eval "require('$1.js')({web:true})"
