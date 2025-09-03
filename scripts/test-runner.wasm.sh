#!/usr/bin/env bash
set -e
node $1.js
node --eval "require('./$1.js')({web:true,worker:{postMessage:()=>{}}})"
qemu-s390x -L /usr/s390x-linux-gnu/ /tmp/node-be/bin/node $1.js
qemu-s390x -L /usr/s390x-linux-gnu/ /tmp/node-be/bin/node --eval "require('./$1.js')({web:true,worker:{postMessage:()=>{}}})"
