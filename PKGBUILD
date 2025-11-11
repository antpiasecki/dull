pkgname=dull-git
pkgver=0.1
pkgrel=1
pkgdesc="Desktop app for securely storing sensitive files"
arch=('x86_64')
url="https://github.com/antpiasecki/dull"
depends=('qt6-base' 'botan')
makedepends=('cmake' 'git')
source=("git+${url}.git")
sha256sums=('SKIP')
options=('!debug')

pkgver() {
  cd "${pkgname%-git}"
  git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
  cd "${pkgname%-git}"
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build -j$(nproc)
}

package() {
  cd "${pkgname%-git}/build"
  make DESTDIR="${pkgdir}" install
}