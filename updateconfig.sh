#!/bin/bash
# StarView Configuration Setup Script

CONFIG_DIR="$HOME/.config/starview"
KEYBINDS_DIR="$CONFIG_DIR/keybinds"
ICONS_DIR="$CONFIG_DIR/icons"
IMAGES_DIR="$CONFIG_DIR/images"

echo "Setting up StarView configuration..."

# Create directory structure
mkdir -p "$CONFIG_DIR"
mkdir -p "$KEYBINDS_DIR"
mkdir -p "$ICONS_DIR"
mkdir -p "$IMAGES_DIR"

# Create main configuration file
cat > "$CONFIG_DIR/starview.toml" << 'EOF'
# StarView Main Configuration
# See: https://github.com/yourname/starview/wiki/Configuration

include = [
    "general.toml",
    "decoration.toml",
    "animation.toml",
    "tiling.toml",
    # "gestures.toml",
    "keybinds/",
    "background.toml",
    "rules.toml",
]
EOF

cat > "$CONFIG_DIR/background.toml" << 'EOF'
[background]
enabled = true
color = "#000000"
image = "/home/link/Pictures/wallpapers/Arch-chan_to.png"
mode = "fill"
EOF

cat > "$CONFIG_DIR/gestures.toml" << 'EOF'
# Gesture Configuration Example

[gestures]
# Sensitivity thresholds
swipe_threshold = 0.3      # How far to swipe (0.0-1.0)
pinch_threshold = 0.15     # How much to pinch (0.0-1.0)
mouse_threshold = 50.0     # Pixels to drag for mouse gestures

# Touchpad Gestures - 3 finger swipes for workspace switching
[[gesture_touchpad]]
fingers = 3
direction = "left"
action = "workspace_next"

[[gesture_touchpad]]
fingers = 3
direction = "right"
action = "workspace_prev"

# Mouse Gestures - Alt+Middle Click drag
[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "up"
action = "maximize"

[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "down"
action = "exec rofi -show drun"

[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "left"
action = "snap_left"

[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "right"
action = "snap_right"

# Super+Right Click for quick apps
[[gesture_mouse]]
button = "right"
modifiers = "Super"
direction = "up"
action = "exec firefox"

[[gesture_mouse]]
button = "right"
modifiers = "Super"
direction = "down"
action = "exec kitty"

[[gesture_mouse]]
button = "right"
modifiers = "Alt"
direction = "left"
action = "exec kitty"

[[gesture_mouse]]
button = "right"
modifiers = "Super"
direction = "right"
action = "exec nautilus"
EOF

# Create general.toml
cat > "$CONFIG_DIR/general.toml" << 'EOF'
[general]
gaps_inner = 5
gaps_outer = 10
border_width = 2
border_color_active = "#89b4fa"
border_color_inactive = "#45475a"
focus_follows_mouse = true
default_mode = "tiling"
resize_step = 50
move_step = 50
EOF

# Create decoration.toml with FULL customization support
cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 34
button_size = 16
button_spacing = 8
corner_radius = 10



# ============================================================================
# BACKGROUND CUSTOMIZATION
# ============================================================================
# Custom titlebar background image (optional)
# If set, this will be used instead of solid colors
# Leave empty or comment out to use colors below
 bg_image = "~/.config/starview/images/titlebar.png"
 bg_image_tile = false  # true = tile, false = stretch

# Fallback solid colors (used if no bg_image)
bg_color = "#1e1e2e"
bg_color_inactive = "#313244"

# ============================================================================
# TEXT/FONT CUSTOMIZATION
# ============================================================================
font = "Inter"           # Font family
font_size = 12           # Font size in pixels
font_weight = 600        # 100-900 (400=normal, 700=bold)
font_italic = false      # Enable italic style

title_color = "#cdd6f4"
title_color_inactive = "#6c7086"

# ============================================================================
# BUTTON ICON CUSTOMIZATION
# ============================================================================
# Custom PNG icons for buttons (optional)
# If set, these will be used instead of drawn icons
# Leave empty or comment out to use default drawn icons

# Example with custom icons:
 icon_close = "~/.config/starview/icons/close.png"
 icon_maximize = "~/.config/starview/icons/maximize.png"
 icon_minimize = "~/.config/starview/icons/minimize.png"

# Fallback button colors (used if no custom icons)
close_color = "#f38ba8"
close_hover = "#eba0ac"
maximize_color = "#a6e3a1"
maximize_hover = "#94e2d5"
minimize_color = "#f9e2af"
minimize_hover = "#f5c2e7"

buttons_left = false     # false = right side (Windows), true = left side (macOS)

# ============================================================================
# SHADOWS
# ============================================================================
[decoration.shadow]
enabled = true
offset_x = 0          # Horizontal offset
offset_y = 8          # Vertical offset (drop shadow)
blur_radius = 20      # Blur amount
opacity = 0.5         # 0.0 - 1.0
color = "#000000"     # Shadow color

[decoration.shadow_inactive]
enabled = true
offset_x = 0
offset_y = 4
blur_radius = 15
opacity = 0.3
color = "#000000"
EOF

# Create a preset configs directory
mkdir -p "$CONFIG_DIR/presets"

# macOS-style preset
cat > "$CONFIG_DIR/presets/macos.toml" << 'EOF'
[decoration]
enabled = true
height = 38
button_size = 13
button_spacing = 8
corner_radius = 12

bg_color = "#ebebed"
bg_color_inactive = "#f6f6f7"

font = "SF Pro Display"
font_size = 13
font_weight = 600
font_italic = false

title_color = "#000000"
title_color_inactive = "#00000060"

# macOS traffic lights
close_color = "#fc615d"
close_hover = "#fc615d"
minimize_color = "#fdbe40"
minimize_hover = "#fdbe40"
maximize_color = "#34c748"
maximize_hover = "#34c748"

buttons_left = true  # macOS has buttons on left
EOF

# Windows 11 style preset
cat > "$CONFIG_DIR/presets/windows11.toml" << 'EOF'
[decoration]
enabled = true
height = 32
button_size = 46
button_spacing = 0
corner_radius = 8

bg_color = "#f3f3f3"
bg_color_inactive = "#fafafa"

font = "Segoe UI Variable"
font_size = 12
font_weight = 600

title_color = "#000000e6"
title_color_inactive = "#00000066"

close_color = "#00000000"
close_hover = "#c42b1c"
maximize_color = "#00000000"
maximize_hover = "#0000000d"
minimize_color = "#00000000"
minimize_hover = "#0000000d"

buttons_left = false
EOF

# Minimal/Hacker preset
cat > "$CONFIG_DIR/presets/minimal.toml" << 'EOF'
[decoration]
enabled = true
height = 28
button_size = 18
button_spacing = 6
corner_radius = 0

bg_color = "#1a1b26"
bg_color_inactive = "#16161e"

font = "JetBrains Mono"
font_size = 10
font_weight = 500

title_color = "#a9b1d6"
title_color_inactive = "#565f89"

close_color = "#f7768e"
close_hover = "#f7768e20"
maximize_color = "#9ece6a"
maximize_hover = "#9ece6a20"
minimize_color = "#e0af68"
minimize_hover = "#e0af6820"

buttons_left = false
EOF

# Create animation.toml
cat > "$CONFIG_DIR/animation.toml" << 'EOF'
[animation]
enabled = true
duration = 200

window_open = "zoom"
window_close = "fade"
window_move = "slide"
window_resize = "none"
workspace_switch = "slide"

curve = "ease_out"
fade_min = 0.0
zoom_min = 0.8
EOF

# Create tiling.toml
cat > "$CONFIG_DIR/tiling.toml" << 'EOF'
[tiling]
master_ratio = 0.55
master_count = 1
EOF

# Create keybind files
cat > "$KEYBINDS_DIR/apps.toml" << 'EOF'
[keybinds]
"Alt+Return" = "spawn kitty"
"Alt+Shift+Return" = "spawn alacritty"
"Alt+d" = "spawn wofi --show drun"
"Alt+b" = "spawn firefox"
"Alt+e" = "spawn thunar"
EOF

cat > "$KEYBINDS_DIR/windows.toml" << 'EOF'
[keybinds]
"Alt+q" = "close"
"Alt+f" = "fullscreen"
"Alt+Space" = "toggle_floating"
"Alt+m" = "minimize"
"Alt+Shift+m" = "maximize"
"Alt+Left" = "snap_left"
"Alt+Right" = "snap_right"
"Alt+c" = "center"
EOF

cat > "$KEYBINDS_DIR/focus.toml" << 'EOF'
[keybinds]
"Alt+h" = "focus left"
"Alt+j" = "focus down"
"Alt+k" = "focus up"
"Alt+l" = "focus right"
"Alt+Tab" = "focus_next"
"Alt+Shift+Tab" = "focus_prev"
EOF

cat > "$KEYBINDS_DIR/move.toml" << 'EOF'
[keybinds]
"Alt+Shift+h" = "move left"
"Alt+Shift+j" = "move down"
"Alt+Shift+k" = "move up"
"Alt+Shift+l" = "move right"
EOF

cat > "$KEYBINDS_DIR/resize.toml" << 'EOF'
[keybinds]
"Alt+Ctrl+h" = "resize_shrink_width"
"Alt+Ctrl+l" = "resize_grow_width"
"Alt+Ctrl+k" = "resize_shrink_height"
"Alt+Ctrl+j" = "resize_grow_height"
EOF

cat > "$KEYBINDS_DIR/workspaces.toml" << 'EOF'
[keybinds]
"Alt+1" = "workspace 1"
"Alt+2" = "workspace 2"
"Alt+3" = "workspace 3"
"Alt+4" = "workspace 4"
"Alt+5" = "workspace 5"
"Alt+6" = "workspace 6"
"Alt+7" = "workspace 7"
"Alt+8" = "workspace 8"
"Alt+9" = "workspace 9"

"Alt+Shift+1" = "move_to_workspace 1"
"Alt+Shift+2" = "move_to_workspace 2"
"Alt+Shift+3" = "move_to_workspace 3"
"Alt+Shift+4" = "move_to_workspace 4"
"Alt+Shift+5" = "move_to_workspace 5"

"Alt+bracketleft" = "workspace_prev"
"Alt+bracketright" = "workspace_next"
EOF

cat > "$KEYBINDS_DIR/layout.toml" << 'EOF'
[keybinds]
"Alt+t" = "mode_tiling"
"Alt+w" = "mode_floating"
"Alt+grave" = "toggle_mode"
"Alt+i" = "inc_master_count"
"Alt+o" = "dec_master_count"
"Alt+minus" = "dec_master_ratio"
"Alt+equal" = "inc_master_ratio"
EOF

cat > "$KEYBINDS_DIR/system.toml" << 'EOF'
[keybinds]
"Alt+Shift+r" = "reload_config"
"Alt+Shift+e" = "exit"
"Print" = "spawn grim ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
"Shift+Print" = "spawn grim -g \"$(slurp)\" ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
EOF

# Create rules.toml
cat > "$CONFIG_DIR/rules.toml" << 'EOF'
[[rules]]
app_id = "firefox"
workspace = 2

[[rules]]
title = "*Picture-in-Picture*"
floating = true

[[rules]]
app_id = "mpv"
floating = true

[[rules]]
app_id = "pavucontrol"
floating = true
EOF

# Create a README
cat > "$CONFIG_DIR/README.md" << 'EOF'
# StarView Configuration

## Titlebar Customization

### Using Custom Icons
1. Place 16x16 PNG icons in `~/.config/starview/icons/`
2. Edit `decoration.toml` and uncomment the icon paths:
```toml
icon_close = "~/.config/starview/icons/close.png"
icon_maximize = "~/.config/starview/icons/maximize.png"
icon_minimize = "~/.config/starview/icons/minimize.png"
```

### Using Custom Background Images
1. Place titlebar background images in `~/.config/starview/images/`
2. Edit `decoration.toml`:
```toml
bg_image = "~/.config/starview/images/titlebar-gradient.png"
bg_image_tile = true  # or false to stretch
```

### Font Customization
```toml
font = "JetBrains Mono"
font_size = 13
font_weight = 600  # 400=normal, 700=bold
font_italic = false
```

### Quick Presets
To use a preset, copy it to your decoration.toml:
```bash
cp presets/macos.toml decoration.toml
# or
cp presets/windows11.toml decoration.toml
# or
cp presets/minimal.toml decoration.toml
```

Then run: `Alt+Shift+r` to reload config
EOF

echo ""
echo "✓ Configuration created at $CONFIG_DIR"
echo ""
echo "Next steps:"
echo "1. Download icon/image assets:"
echo "   bash download_assets.sh"
echo ""
echo "2. Edit decoration.toml to customize:"
echo "   - Uncomment icon_* lines to use custom icons"
echo "   - Uncomment bg_image to use custom background"
echo "   - Adjust font, colors, sizes"
echo ""
echo "3. Try a preset:"
echo "   cp $CONFIG_DIR/presets/macos.toml $CONFIG_DIR/decoration.toml"
echo ""
echo "4. Start StarView and press Alt+Shift+r to reload config"
