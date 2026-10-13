#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
version=$(sed -n 's/^pkgver=//p' "$root/PKGBUILD")
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

git -C "$root" archive --format=tar --prefix="typist101-$version/" HEAD |
  gzip -n > "$work/typist101-$version.tar.gz"
checksum=$(sha256sum "$work/typist101-$version.tar.gz" | cut -d ' ' -f 1)
sed "s/sha256sums=('SKIP')/sha256sums=('$checksum')/" "$root/PKGBUILD" > "$work/PKGBUILD"

mkdir -p "$root/dist"
(cd "$work" && PKGDEST="$root/dist" makepkg --noconfirm --force)
