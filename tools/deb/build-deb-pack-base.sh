#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd -- "$SCRIPT_DIR/../.." && pwd)
PNAME=$(mktemp -d "$ROOT/.hiasm-pack-base.XXXXXX")
CONTROL_DIR="$SCRIPT_DIR/hiasm-pack"
PACK_DIR="$ROOT/packs/_base"
VERSION_DIR="$ROOT/pack-ver"

mkdir -p "$VERSION_DIR"
if [[ -f "$VERSION_DIR/base" ]]; then
    VERSION=$(( $(<"$VERSION_DIR/base") + 1 ))
else
    VERSION=1
fi
printf "%s\n" "$VERSION" > "$VERSION_DIR/base"

cleanup() {
    rm -rf "$PNAME"
}
trap cleanup EXIT INT TERM

mkdir -p "$PNAME/DEBIAN" "$PNAME/usr/share/hiasm/packs/_base"
cp "$CONTROL_DIR"/co* "$PNAME/DEBIAN/"
cp -R "$PACK_DIR/." "$PNAME/usr/share/hiasm/packs/_base/"

(cd "$PNAME/usr/share/hiasm/packs/_base" && sh ./sql_to_db.sh)

sed -i "s/%version%$/0.0.$VERSION/" "$PNAME/DEBIAN/control"
sed -i "s/%pack%$/base/" "$PNAME/DEBIAN/control"
sed -i "s/%arch%$/all/" "$PNAME/DEBIAN/control"
sed -i "s/^Depends: $/Origin: vdv/" "$PNAME/DEBIAN/control"

(cd "$PNAME" && md5deep -r usr > DEBIAN/md5sums)
fakeroot dpkg-deb --build "$PNAME" "$ROOT/hiasm-pack-base.deb"
