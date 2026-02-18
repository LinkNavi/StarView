#!/bin/bash
# download_assets.sh - Download example titlebar assets

ICONS_DIR="$HOME/.config/starview/icons"
IMAGES_DIR="$HOME/.config/starview/images"

mkdir -p "$ICONS_DIR"
mkdir -p "$IMAGES_DIR"

echo "Creating example button icons..."

# Simple colored circle icons using ImageMagick
if command -v convert &> /dev/null; then
    # Red close button
    convert -size 16x16 xc:none -fill "#f38ba8" -draw "circle 8,8 8,0" "$ICONS_DIR/close.png"
    
    # Green maximize button
    convert -size 16x16 xc:none -fill "#a6e3a1" -draw "circle 8,8 8,0" "$ICONS_DIR/maximize.png"
    
    # Yellow minimize button
    convert -size 16x16 xc:none -fill "#f9e2af" -draw "circle 8,8 8,0" "$ICONS_DIR/minimize.png"
    
    # Subtle gradient background
    convert -size 400x34 gradient:"#313244"-"#1e1e2e" "$IMAGES_DIR/titlebar-gradient.png"
    
    echo "✓ Created example icons and images"
else
    echo "ImageMagick not found. Install with: sudo pacman -S imagemagick"
    echo "Or download pre-made icons manually"
fi

echo ""
echo "Assets created in:"
echo "  Icons: $ICONS_DIR"
echo "  Images: $IMAGES_DIR"
