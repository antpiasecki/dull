# dull

Desktop app for securely storing sensitive files

## Features
* **Pretty usable UI**
* **Overkill encryption:** XChaCha20-Poly1305 + Argon2id(m=1GB,t=8,p=4) key derivation
* **Cross-platform-ish:** Supports Linux and Windows

## Building

### Debian/Ubuntu
```
sudo apt update && sudo apt install build-essential qt6-base-dev libbotan-3-dev cmake

cmake -S . -B build
cmake --build build -j $(nproc)
./build/dull
```

### Windows / MSYS2
```
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-tools mingw-w64-x86_64-libbotan

cmake -S . -B build -DBOTAN_INCLUDE_DIRS=/mingw64/include/botan-3 -DBOTAN_LIBRARIES=/mingw64/lib/libbotan-3.a
cmake --build build -j $(nproc)
./build/dull
```