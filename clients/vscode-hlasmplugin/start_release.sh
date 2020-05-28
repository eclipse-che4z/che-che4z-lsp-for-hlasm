#!/bin/bash
set -e
BRANCH=`git rev-parse --abbrev-ref HEAD`
if [ "$BRANCH" != "development" ]; then
    echo "Start release can be done only from development branch, you are on $BRANCH."
    exit 1
fi

git pull

sed -i '3s/^/  - development\n/' .releaserc.yaml

npx semantic-release --dry-run



VERSION=`node -e "console.log(require('./package.json').version)"`

git reset --hard
git push origin HEAD:release-$VERSION