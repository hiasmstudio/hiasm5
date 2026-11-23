#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
PNAME=$(mktemp -d "$ROOT/.hiasm-common.XXXXXX")
RES="$ROOT/resources"
DEST="$PNAME/usr/share"

cleanup() {
    rm -rf "$PNAME"
}
trap cleanup EXIT INT TERM

cp -R "$SCRIPT_DIR/hiasm-common-control/." "$PNAME/"

mkdir -p "$DEST/hiasm/int" "$DEST/applications" "$DEST/pixmaps"
python3 "$ROOT/tools/build_database.py" "$DEST/hiasm/int/hiasm.db" "$RES/hiasm.db" "$ROOT/packs/cpp/cpp_package.sql"
cp "$RES/hiasm.sql" "$DEST/hiasm/int/"
cp "$RES/linux/applications/hiasm.desktop" "$DEST/applications/"
cp "$RES/hiasm.png" "$DEST/pixmaps/"

VERSION1=$(awk '$1 == "#define" && $2 == "HIASM_VERSION_MAJOR" {print $3}' "$ROOT/src/share.h")
VERSION2=$(awk '$1 == "#define" && $2 == "HIASM_VERSION_MINOR" {print $3}' "$ROOT/src/share.h")
VERSION3=$(awk '$1 == "#define" && $2 == "HIASM_VERSION_BUILD" {print $3}' "$ROOT/src/share.h")
VERSION4=$(awk '$1 == "#define" && $2 == "HIASM_VERSION_TWEAK" {print $3}' "$ROOT/src/share.h")
VERSION="$VERSION1.$VERSION2.$VERSION3.$VERSION4"

sed -i "s/%version%$/$VERSION/" "$PNAME/DEBIAN/control"

(cd "$PNAME" && md5deep -r usr > DEBIAN/md5sums)
fakeroot dpkg-deb --build "$PNAME" "$ROOT/hiasm-common.deb"
