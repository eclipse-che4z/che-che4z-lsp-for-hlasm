#!/usr/bin/env bash
set -e
node $1.js
node --eval "require('./$1.js')({web:true,worker:{postMessage:()=>{}}})"
