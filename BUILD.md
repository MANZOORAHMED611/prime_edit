# Building PrimeEdit

## Prerequisites

### Ubuntu/Debian
```bash
# Qt6 (recommended). libgl-dev is required: without GL/gl.h, Qt6Gui's
# CMake config fails and the build falls back to (and errors on) Qt5.
sudo apt install qt6-base-dev libqt6svg6-dev libgl-dev cmake build-essential

# Or Qt5
sudo apt install qtbase5-dev qt5-qmake qttools5-dev cmake build-essential
```

### Fedora
```bash
# Qt6
sudo dnf install qt6-qtbase-devel cmake gcc-c++

# Or Qt5
sudo dnf install qt5-qtbase-devel cmake gcc-c++
```

### Arch Linux
```bash
# Qt6
sudo pacman -S qt6-base cmake base-devel

# Or Qt5
sudo pacman -S qt5-base cmake base-devel
```

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run
./PrimeEdit
```

## Testing

Both suites are built by default (`-DPRIMEEDIT_BUILD_TESTS=OFF` to skip) and
run headless with an isolated HOME, so they never touch your real PrimeEdit
settings, session or unsaved-document cache:

```bash
ctest --output-on-failure
```

## Installing

```bash
sudo make install
```

## Packaging

### AppImage
```bash
# Install linuxdeployqt first
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
make install DESTDIR=AppDir
linuxdeployqt AppDir/usr/share/applications/prime-edit.desktop -appimage
```

### Debian Package
```bash
# Create package structure
mkdir -p deb/DEBIAN
mkdir -p deb/usr/bin
mkdir -p deb/usr/share/applications
mkdir -p deb/usr/share/icons/hicolor/256x256/apps

cp PrimeEdit deb/usr/bin/
cp ../resources/prime-edit.desktop deb/usr/share/applications/
cp ../resources/icons/prime_edit_icon.png deb/usr/share/icons/hicolor/256x256/apps/prime-edit.png

cat > deb/DEBIAN/control << EOF
Package: prime-edit
Version: 1.0.0
Section: editors
Priority: optional
Architecture: amd64
Depends: libqt6core6, libqt6gui6, libqt6widgets6
Maintainer: Your Name <your@email.com>
Description: The editor for structured knowledge work
 A native editor for structured documents, where content is
 validated, enforced, and executable.
EOF

dpkg-deb --build deb prime-edit_1.0.0_amd64.deb
```
