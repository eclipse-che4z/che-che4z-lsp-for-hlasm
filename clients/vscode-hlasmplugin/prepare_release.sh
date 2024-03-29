set -e
set -x
VERSION=$1
CURRENT_BRANCH=$2
RELEASE_NOTES=$3
OLD_VERSION=$4
DATE=$5

{ printf "%s" "$RELEASE_NOTES"; cat ../../CHANGELOG.md; } > tmp.md
mv tmp.md ../../CHANGELOG.md

case "$VERSION" in
    *-alpha*)
        sed -i 's/"version": ".*"/"version": "'$VERSION+$DATE'"/g' package.json
        ;;
    *)
        sed -i 's/"version": ".*"/"version": "'$VERSION'"/g' package.json
        ;;
esac
sed -i 's@\*\*\*\*Unreleased\*\*\*\*@['$VERSION'](https://github.com/eclipse-che4z/che-che4z-lsp-for-hlasm/compare/'$OLD_VERSION'...'$VERSION') ('`date +%Y-%m-%d`')@g' CHANGELOG.md
