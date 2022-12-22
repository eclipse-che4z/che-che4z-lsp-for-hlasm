#!/bin/bash
set -e
BRANCH=`git rev-parse --abbrev-ref HEAD`
if [ "$BRANCH" != "development" ]; then
    echo "Start release can be done only from development branch, you are on $BRANCH."
    exit 1
fi

git reset --hard && git pull --ff-only && git push origin HEAD:release-next
