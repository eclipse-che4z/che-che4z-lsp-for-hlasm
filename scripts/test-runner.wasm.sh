#!/usr/bin/env bash
set -e
node --experimental-wasm-eh $1.js
node --experimental-wasm-eh --eval "import('$1.mjs').then(m=>{m.default({web:true})})"
