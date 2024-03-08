#!/usr/bin/env sh
set -e
apk update && apk add --no-cache linux-headers git g++ cmake ninja
