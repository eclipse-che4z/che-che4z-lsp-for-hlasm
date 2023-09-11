#!/usr/bin/env bash
set -e
node --experimental-wasm-threads --experimental-wasm-bulk-memory $1.js
node --experimental-wasm-threads --experimental-wasm-bulk-memory --input-type=module --eval "import('$1.mjs').then(m=>{m.default({web:true})})"
