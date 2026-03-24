# Building Notepad Supreme

## Prerequisites

### Ubuntu/Debian
```bash
# Qt6 (recommended)
sudo apt install qt6-base-dev qt6-tools-dev cmake build-essential

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
./notepad-supreme
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
linuxdeployqt AppDir/usr/share/applications/notepad-supreme.desktop -appimage
```

### Debian Package
```bash
# Create package structure
mkdir -p deb/DEBIAN
mkdir -p deb/usr/bin
mkdir -p deb/usr/share/applications
mkdir -p deb/usr/share/icons/hicolor/256x256/apps

cp notepad-supreme deb/usr/bin/
cp ../resources/notepad-supreme.desktop deb/usr/share/applications/
cp ../resources/icons/notepad_supreme_icon.png deb/usr/share/icons/hicolor/256x256/apps/notepad-supreme.png

cat > deb/DEBIAN/control << EOF
Package: notepad-supreme
Version: 1.0.0
Section: editors
Priority: optional
Architecture: amd64
Depends: libqt6core6, libqt6gui6, libqt6widgets6
Maintainer: Your Name <your@email.com>
Description: Native Linux text editor
 A native Linux text editor designed to match and exceed
 Notepad++ functionality with modern features.
EOF

dpkg-deb --build deb notepad-supreme_1.0.0_amd64.deb
```
