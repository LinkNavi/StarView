#!/bin/bash
set -e

echo "=== Installing ALL dependencies for wlroots 0.18 ==="
echo ""

# Install base build dependencies
echo "Step 1: Installing base dependencies..."
sudo apt update
sudo apt install -y \
  meson ninja-build pkg-config cmake git \
  libffi-dev libxml2-dev libexpat1-dev \
  libegl1-mesa-dev libgles2-mesa-dev \
  libgbm-dev libinput-dev libxkbcommon-dev \
  libudev-dev libpixman-1-dev libseat-dev \
  libvulkan-dev glslang-tools hwdata \
  libxcb-dri3-dev libxcb-present-dev \
  libxcb-composite0-dev libxcb-render-util0-dev \
  libxcb-ewmh-dev libxcb-icccm4-dev \
  libxcb-xinput-dev libxcb-xfixes0-dev libxcb-res0-dev \
  libpciaccess-dev libcairo2-dev valgrind

echo ""
echo "Step 2: Building libdrm 2.4.122..."
cd /tmp
rm -rf drm
git clone https://gitlab.freedesktop.org/mesa/drm.git
cd drm
git checkout libdrm-2.4.122

meson setup build/ --prefix=/usr/local -Dudev=true
ninja -C build/
sudo ninja -C build/ install
sudo ldconfig

echo ""
echo "Step 3: Building Wayland 1.23..."
cd /tmp
rm -rf wayland
git clone https://gitlab.freedesktop.org/wayland/wayland.git
cd wayland
git checkout 1.23.0

meson setup build/ --prefix=/usr/local -Ddocumentation=false
ninja -C build/
sudo ninja -C build/ install
sudo ldconfig

echo ""
echo "Step 4: Building wayland-protocols 1.36..."
cd /tmp
rm -rf wayland-protocols
git clone https://gitlab.freedesktop.org/wayland/wayland-protocols.git
cd wayland-protocols
git checkout 1.36

meson setup build/ --prefix=/usr/local
ninja -C build/
sudo ninja -C build/ install

echo ""
echo "Step 5: Checking for optional dependencies..."

# Try to install libdisplay-info
if ! pkg-config --exists libdisplay-info 2>/dev/null; then
    echo "  Building libdisplay-info..."
    cd /tmp
    rm -rf libdisplay-info
    git clone https://gitlab.freedesktop.org/emersion/libdisplay-info.git
    cd libdisplay-info
    meson setup build/ --prefix=/usr/local
    ninja -C build/
    sudo ninja -C build/ install
    sudo ldconfig
else
    echo "  libdisplay-info already installed"
fi

# Try to install libliftoff
if ! pkg-config --exists libliftoff 2>/dev/null; then
    echo "  Building libliftoff..."
    cd /tmp
    rm -rf libliftoff
    git clone https://gitlab.freedesktop.org/emersion/libliftoff.git
    cd libliftoff
    meson setup build/ --prefix=/usr/local
    ninja -C build/
    sudo ninja -C build/ install
    sudo ldconfig
else
    echo "  libliftoff already installed"
fi

echo ""
echo "Step 6: Building wlroots 0.18..."
cd /tmp
rm -rf wlroots
git clone https://gitlab.freedesktop.org/wlroots/wlroots.git
cd wlroots
git checkout 0.18.1

# Set PKG_CONFIG_PATH to find our new libraries
export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

meson setup build/ --prefix=/usr/local
ninja -C build/
sudo ninja -C build/ install
sudo ldconfig

echo ""
echo "Step 7: Cleanup..."
cd ~
rm -rf /tmp/drm /tmp/wayland /tmp/wayland-protocols /tmp/wlroots /tmp/libdisplay-info /tmp/libliftoff

echo ""
echo "=========================================="
echo "✓ Installation complete!"
echo "=========================================="
echo ""
echo "Installed versions:"
echo -n "libdrm: "
pkg-config --modversion libdrm
echo -n "wayland-server: "
pkg-config --modversion wayland-server
echo -n "wayland-protocols: "
pkg-config --modversion wayland-protocols
echo -n "wlroots-0.18: "
pkg-config --modversion wlroots-0.18
echo ""

# Add to bashrc for permanent environment variables
if ! grep -q "PKG_CONFIG_PATH.*usr/local" ~/.bashrc; then
    echo "" >> ~/.bashrc
    echo "# Wayland/wlroots environment" >> ~/.bashrc
    echo 'export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH' >> ~/.bashrc
    echo 'export LD_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu:/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
    echo "Added environment variables to ~/.bashrc"
fi

echo ""
echo "IMPORTANT: Run this command to update your current shell:"
echo "  source ~/.bashrc"
echo ""
echo "Then build your compositor:"
echo "  cd ~/Programming/StarView"
echo "  rm -rf build"
echo "  meson setup build"
echo "  ninja -C build"
echo ""
