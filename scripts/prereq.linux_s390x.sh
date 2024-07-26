#!/usr/bin/env sh
set -e
apk update && apk add --no-cache git cmake ninja qemu-s390x tar
