#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
"$root/scripts/package-arch.sh"
package=$(cd "$root" && PKGDEST="$root/dist" makepkg --packagelist)
exec sudo pacman -U "$package"
