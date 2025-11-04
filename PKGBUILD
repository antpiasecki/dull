pkgname=dull
pkgver=0.1
pkgrel=1
pkgdesc="Desktop app for securely storing sensitive files"
arch=('x86_64')
url="https://github.com/antpiasecki/dull"
depends=('qt6-base' 'botan')
makedepends=('cmake')
source=("${url}/archive/refs/tags/${pkgver}.tar.gz")
sha256sums=('SKIP')
DEBUGPKG=()

build() {
  cd "${pkgname}-${pkgver}"
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build -j$(nproc)
}

package() {
  cd "${pkgname}-${pkgver}/build"
  make DESTDIR="${pkgdir}" install
}