pkgname=typist101
pkgver=0.1.0
pkgrel=1
pkgdesc='Typing practice with large text and visual presets'
url='https://github.com/konakona418/typist-101'
arch=('x86_64')
license=('GPL-3.0-only' 'Zlib')
depends=('sdl3' 'fontconfig' 'libepoxy')
makedepends=('cmake' 'ninja' 'gcc' 'pkgconf')
options=('!debug')
source=("$pkgname-$pkgver.tar.gz")
sha256sums=('SKIP') # scripts/package-arch.sh sets the archive checksum.

build() {
  cmake -S "$srcdir/$pkgname-$pkgver" -B "$srcdir/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF -DTYPIST_SDL_IME=ON -DTYPIST_PREFER_SDL3=ON
  cmake --build "$srcdir/build"
}

package() {
  DESTDIR="$pkgdir" cmake --install "$srcdir/build" --component Runtime
  install -Dm644 "$srcdir/$pkgname-$pkgver/LICENSE" \
    "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
  install -Dm644 "$srcdir/$pkgname-$pkgver/vendors/raylib/LICENSE" \
    "$pkgdir/usr/share/licenses/$pkgname/raylib-LICENSE"
}
