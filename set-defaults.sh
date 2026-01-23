#!/bin/bash
# StarView Modern Theme Configuration Script
# Choose between: default, macos, windows11, gnome, minimal

CONFIG_DIR="$HOME/.config/starview"
KEYBINDS_DIR="$CONFIG_DIR/keybinds"

echo "=== StarView Modern Theme Setup ==="
echo ""
echo "Available themes:"
echo "  1) default   - Modern Catppuccin Mocha with glow effects"
echo "  2) macos     - macOS Big Sur style traffic lights"
echo "  3) windows11 - Windows 11 Fluent Design"
echo "  4) gnome     - GNOME 45+ Libadwaita style"
echo "  5) minimal   - Ultra-minimal Tokyo Night theme"
echo ""
echo "Select theme (1-5) [default: 1]: "
read -r theme_choice

case "$theme_choice" in
    2) THEME="macos" ;;
    3) THEME="windows11" ;;
    4) THEME="gnome" ;;
    5) THEME="minimal" ;;
    *) THEME="default" ;;
esac

echo "Setting up '$THEME' theme..."

# Create directory structure
mkdir -p "$CONFIG_DIR"
mkdir -p "$KEYBINDS_DIR"

# Create main configuration
cat > "$CONFIG_DIR/starview.toml" << 'EOF'
# StarView Modern Configuration
# See: https://github.com/yourname/starview/wiki/Configuration

include = [
    "general.toml",
    "decoration.toml",
    "animation.toml",
    "tiling.toml",
    "gestures.toml",
    "keybinds/",
    "rules.toml",
]
EOF

# Theme-specific settings
case "$THEME" in
    "macos")
        cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 38
button_size = 13
button_spacing = 8
corner_radius = 12

# macOS Big Sur style
bg_color = "#ebebed"
bg_color_inactive = "#f6f6f7"
title_color = "#000000"
title_color_inactive = "#00000060"

# Traffic light buttons (left side)
close_color = "#fc615d"
close_hover = "#fc615d"
maximize_color = "#34c748"
maximize_hover = "#34c748"
minimize_color = "#fdbe40"
minimize_hover = "#fdbe40"

font = "SF Pro Display, sans-serif"
font_size = 13
buttons_left = true

# Modern shadows
shadow_enabled = true
shadow_blur = 8
shadow_offset = 2
EOF
        ;;
        
    "windows11")
        cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 32
button_size = 46
button_spacing = 0
corner_radius = 8

# Windows 11 Fluent Design (Mica)
bg_color = "#f3f3f3fa"
bg_color_inactive = "#fafafafe"
title_color = "#000000e6"
title_color_inactive = "#00000066"

# Accent-colored close button
close_color = "#c42b1c"
close_hover = "#c42b1c"
maximize_color = "#0000000d"
maximize_hover = "#0000000d"
minimize_color = "#0000000d"
minimize_hover = "#0000000d"

font = "Segoe UI Variable, sans-serif"
font_size = 12
buttons_left = false
EOF
        ;;
        
    "gnome")
        cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 40
button_size = 22
button_spacing = 6
corner_radius = 12

# GNOME Libadwaita dark
bg_color = "#303030"
bg_color_inactive = "#242424"
title_color = "#fffffffa"
title_color_inactive = "#ffffff99"

# Modern circular buttons
close_color = "#ff6b6b"
close_hover = "#ff6b6b"
maximize_color = "#ffffff1a"
maximize_hover = "#ffffff2a"
minimize_color = "#ffffff1a"
minimize_hover = "#ffffff2a"

font = "Cantarell, Inter, sans-serif"
font_size = 11
buttons_left = false

# Subtle gradient
gradient_enabled = true
EOF
        ;;
        
    "minimal")
        cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 28
button_size = 18
button_spacing = 6
corner_radius = 0

# Tokyo Night minimal
bg_color = "#1a1b26"
bg_color_inactive = "#16161e"
title_color = "#a9b1d6"
title_color_inactive = "#565f89"

# Flat accent colors
close_color = "#f7768e"
close_hover = "#f7768e20"
maximize_color = "#9ece6a"
maximize_hover = "#9ece6a20"
minimize_color = "#e0af68"
minimize_hover = "#e0af6820"

font = "JetBrains Mono, monospace"
font_size = 10
buttons_left = false
EOF
        ;;
        
    *)  # default - Modern Catppuccin Mocha
        cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 34
button_size = 16
button_spacing = 8
corner_radius = 10

# Modern Catppuccin Mocha with glow
bg_color = "#1e1e2e"
bg_color_inactive = "#181825"
title_color = "#cdd6f4"
title_color_inactive = "#6c7086"

# Glowing buttons with shadow
close_color = "#f38ba8"
close_hover = "#eba0ac"
maximize_color = "#a6e3a1"
maximize_hover = "#94e2d5"
minimize_color = "#f9e2af"
minimize_hover = "#f5c2e7"

font = "Inter, sans-serif"
font_size = 12
buttons_left = false

# Modern effects
glow_enabled = true
glow_intensity = 0.4
gradient_enabled = true
EOF
        ;;
esac

# General settings (theme-aware)
if [ "$THEME" = "macos" ] || [ "$THEME" = "windows11" ]; then
    # Light themes
    cat > "$CONFIG_DIR/general.toml" << 'EOF'
[general]
gaps_inner = 10
gaps_outer = 15
border_width = 1
border_color_active = "#007aff"
border_color_inactive = "#e0e0e0"
focus_follows_mouse = true
default_mode = "tiling"
resize_step = 50
move_step = 50
EOF
else
    # Dark themes
    cat > "$CONFIG_DIR/general.toml" << 'EOF'
[general]
gaps_inner = 8
gaps_outer = 12
border_width = 2
border_color_active = "#89b4fa"
border_color_inactive = "#45475a"
focus_follows_mouse = true
default_mode = "tiling"
resize_step = 50
move_step = 50
EOF
fi

# Modern animations
cat > "$CONFIG_DIR/animation.toml" << 'EOF'
[animation]
enabled = true
duration = 220

# Smooth modern animations
window_open = "zoom"
window_close = "fade"
window_move = "slide"
window_resize = "none"
workspace_switch = "slide"

# Smooth easing curves
curve = "ease_in_out"
fade_min = 0.0
zoom_min = 0.88

# Spring animations for macOS-like feel
spring_mass = 1.0
spring_stiffness = 180
spring_damping = 20
EOF

cat > "$CONFIG_DIR/tiling.toml" << 'EOF'
[tiling]
# Dwindle layout (Hyprland-style)
master_ratio = 0.55
master_count = 1

# Smart gaps
smart_gaps = true
smart_borders = true
EOF

# Enhanced gesture support
cat > "$CONFIG_DIR/gestures.toml" << 'EOF'
# Modern Gesture Configuration

[gestures]
# Fine-tuned thresholds
swipe_threshold = 0.25
pinch_threshold = 0.12
mouse_threshold = 45.0

# Natural scrolling
natural_scroll = true
invert_scroll = false

# Touchpad Gestures - macOS-like
[[gesture_touchpad]]
fingers = 3
direction = "left"
action = "workspace_next"

[[gesture_touchpad]]
fingers = 3
direction = "right"
action = "workspace_prev"

[[gesture_touchpad]]
fingers = 3
direction = "up"
action = "maximize"

[[gesture_touchpad]]
fingers = 3
direction = "down"
action = "exec rofi -show drun"

# 4-finger gestures
[[gesture_touchpad]]
fingers = 4
direction = "up"
action = "exec rofi -show window"

[[gesture_touchpad]]
fingers = 4
direction = "down"
action = "minimize"

# Pinch gestures
[[gesture_touchpad]]
fingers = 2
direction = "in"
action = "toggle_floating"

# Mouse Gestures - Alt+Middle for window management
[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "up"
action = "maximize"

[[gesture_mouse]]
button = "middle"
modifiers = "Alt"
direction = "down"
action = "minimize"

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

# Super+Right Click for quick launcher
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
modifiers = "Super"
direction = "left"
action = "exec thunar"

[[gesture_mouse]]
button = "right"
modifiers = "Super"
direction = "right"
action = "exec code"

# Alt+Right for workspace switching
[[gesture_mouse]]
button = "right"
modifiers = "Alt"
direction = "left"
action = "workspace_prev"

[[gesture_mouse]]
button = "right"
modifiers = "Alt"
direction = "right"
action = "workspace_next"
EOF

# Create modern keybinds
cat > "$KEYBINDS_DIR/apps.toml" << 'EOF'
[keybinds]
# Terminals
"Super+Return" = "spawn kitty"
"Super+Shift+Return" = "spawn alacritty"

# Launchers
"Super+d" = "spawn rofi -show drun"
"Super+r" = "spawn rofi -show run"
"Super+slash" = "spawn rofi -show window"

# Common apps
"Super+b" = "spawn firefox"
"Super+e" = "spawn thunar"
"Super+c" = "spawn code"
"Super+n" = "spawn obsidian"
EOF

cat > "$KEYBINDS_DIR/windows.toml" << 'EOF'
[keybinds]
# Window management
"Super+q" = "close"
"Super+f" = "fullscreen"
"Super+Space" = "toggle_floating"
"Super+m" = "minimize"
"Super+Shift+m" = "maximize"

# Snapping (Windows-like)
"Super+Left" = "snap_left"
"Super+Right" = "snap_right"
"Super+Up" = "maximize"
"Super+Down" = "minimize"
"Super+Shift+c" = "center"
EOF

cat > "$KEYBINDS_DIR/focus.toml" << 'EOF'
[keybinds]
# Vim-style navigation
"Super+h" = "focus left"
"Super+j" = "focus down"
"Super+k" = "focus up"
"Super+l" = "focus right"

# Tab navigation
"Super+Tab" = "focus_next"
"Super+Shift+Tab" = "focus_prev"

# Monitor focus
"Super+comma" = "focus_monitor_prev"
"Super+period" = "focus_monitor_next"
EOF

cat > "$KEYBINDS_DIR/move.toml" << 'EOF'
[keybinds]
# Move/swap windows (tiling)
"Super+Shift+h" = "swap_left"
"Super+Shift+j" = "swap_down"
"Super+Shift+k" = "swap_up"
"Super+Shift+l" = "swap_right"

# Move windows (floating)
"Super+Ctrl+h" = "move left"
"Super+Ctrl+j" = "move down"
"Super+Ctrl+k" = "move up"
"Super+Ctrl+l" = "move right"

# Preselect position for next window
"Super+Alt+h" = "preselect_left"
"Super+Alt+j" = "preselect_down"
"Super+Alt+k" = "preselect_up"
"Super+Alt+l" = "preselect_right"
EOF

cat > "$KEYBINDS_DIR/resize.toml" << 'EOF'
[keybinds]
# Resize windows
"Super+Ctrl+Shift+h" = "resize_shrink_width"
"Super+Ctrl+Shift+l" = "resize_grow_width"
"Super+Ctrl+Shift+k" = "resize_shrink_height"
"Super+Ctrl+Shift+j" = "resize_grow_height"

# Reset size
"Super+Ctrl+Shift+r" = "resize_reset"
EOF

cat > "$KEYBINDS_DIR/workspaces.toml" << 'EOF'
[keybinds]
# Workspace switching
"Super+1" = "workspace 1"
"Super+2" = "workspace 2"
"Super+3" = "workspace 3"
"Super+4" = "workspace 4"
"Super+5" = "workspace 5"
"Super+6" = "workspace 6"
"Super+7" = "workspace 7"
"Super+8" = "workspace 8"
"Super+9" = "workspace 9"

# Move window to workspace
"Super+Shift+1" = "move_to_workspace 1"
"Super+Shift+2" = "move_to_workspace 2"
"Super+Shift+3" = "move_to_workspace 3"
"Super+Shift+4" = "move_to_workspace 4"
"Super+Shift+5" = "move_to_workspace 5"
"Super+Shift+6" = "move_to_workspace 6"
"Super+Shift+7" = "move_to_workspace 7"
"Super+Shift+8" = "move_to_workspace 8"
"Super+Shift+9" = "move_to_workspace 9"

# Cycle workspaces
"Super+bracketleft" = "workspace_prev"
"Super+bracketright" = "workspace_next"
"Super+Page_Up" = "workspace_prev"
"Super+Page_Down" = "workspace_next"
EOF

cat > "$KEYBINDS_DIR/layout.toml" << 'EOF'
[keybinds]
# Layout modes
"Super+t" = "mode_tiling"
"Super+w" = "mode_floating"
"Super+grave" = "toggle_mode"

# Tiling layout adjustments
"Super+i" = "inc_master_count"
"Super+o" = "dec_master_count"
"Super+minus" = "dec_master_ratio"
"Super+equal" = "inc_master_ratio"

# Rotate layout
"Super+Shift+Space" = "rotate_layout"
EOF

cat > "$KEYBINDS_DIR/system.toml" << 'EOF'
[keybinds]
# System controls
"Super+Shift+r" = "reload_config"
"Super+Shift+e" = "exit"
"Super+Escape" = "spawn swaylock"

# Screenshots (modern)
"Print" = "spawn grim ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
"Shift+Print" = "spawn grim -g \"$(slurp)\" ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
"Super+Shift+s" = "spawn grim -g \"$(slurp)\" - | wl-copy"
"Super+Print" = "spawn grim -g \"$(slurp)\" - | swappy -f -"

# Media controls
"XF86AudioPlay" = "spawn playerctl play-pause"
"XF86AudioNext" = "spawn playerctl next"
"XF86AudioPrev" = "spawn playerctl previous"
"XF86AudioRaiseVolume" = "spawn pactl set-sink-volume @DEFAULT_SINK@ +5%"
"XF86AudioLowerVolume" = "spawn pactl set-sink-volume @DEFAULT_SINK@ -5%"
"XF86AudioMute" = "spawn pactl set-sink-mute @DEFAULT_SINK@ toggle"

# Brightness
"XF86MonBrightnessUp" = "spawn brightnessctl set +5%"
"XF86MonBrightnessDown" = "spawn brightnessctl set 5%-"
EOF

# Modern window rules
cat > "$CONFIG_DIR/rules.toml" << 'EOF'
# Modern Window Rules

# Browser on workspace 2
[[rules]]
app_id = "firefox"
workspace = 2
opacity = 1.0

[[rules]]
app_id = "chromium"
workspace = 2

[[rules]]
app_id = "Google-chrome"
workspace = 2

# Communication on workspace 3
[[rules]]
app_id = "discord"
workspace = 3

[[rules]]
app_id = "Slack"
workspace = 3

[[rules]]
app_id = "teams"
workspace = 3

# Development on workspace 4
[[rules]]
app_id = "code"
workspace = 4

[[rules]]
app_id = "jetbrains"
workspace = 4

# Media players - floating
[[rules]]
app_id = "mpv"
floating = true
width = 1280
height = 720

[[rules]]
app_id = "vlc"
floating = true

# Picture-in-picture always on top
[[rules]]
title = "*Picture-in-Picture*"
floating = true
width = 640
height = 360

[[rules]]
title = "*PiP*"
floating = true

# System utilities - floating and centered
[[rules]]
app_id = "pavucontrol"
floating = true
width = 800
height = 600

[[rules]]
app_id = "blueman-manager"
floating = true
width = 700
height = 500

[[rules]]
app_id = "nm-connection-editor"
floating = true

# File pickers
[[rules]]
title = "*Open File*"
floating = true

[[rules]]
title = "*Save As*"
floating = true

# Preferences/Settings dialogs
[[rules]]
title = "*Preferences*"
floating = true

[[rules]]
title = "*Settings*"
floating = true

# Steam
[[rules]]
app_id = "steam"
workspace = 9
floating = false

[[rules]]
title = "*Steam - News*"
floating = true

# Calculators
[[rules]]
app_id = "gnome-calculator"
floating = true
width = 400
height = 600

[[rules]]
app_id = "org.kde.kcalc"
floating = true

# Terminal dropdown
[[rules]]
app_id = "dropdown-terminal"
floating = true
width = 1600
height = 800

# Notification center
[[rules]]
app_id = "swaync"
floating = true

# Screen sharing indicators
[[rules]]
title = "*is sharing*"
floating = true
width = 300
height = 100
EOF

echo ""
echo "✓ StarView configured with '$THEME' theme!"
echo ""
echo "Theme features:"
case "$THEME" in
    "macos")
        echo "  • macOS Big Sur traffic light buttons"
        echo "  • Smooth gradients and shadows"
        echo "  • Left-aligned window controls"
        echo "  • SF Pro Display font"
        ;;
    "windows11")
        echo "  • Windows 11 Fluent Design"
        echo "  • Mica material effect"
        echo "  • Accent-colored close button"
        echo "  • Segoe UI Variable font"
        ;;
    "gnome")
        echo "  • GNOME Libadwaita dark theme"
        echo "  • Modern circular buttons"
        echo "  • Subtle gradients"
        echo "  • Cantarell font"
        ;;
    "minimal")
        echo "  • Tokyo Night minimal theme"
        echo "  • Flat accent colors"
        echo "  • No decorations, maximum space"
        echo "  • JetBrains Mono font"
        ;;
    *)
        echo "  • Modern Catppuccin Mocha theme"
        echo "  • Glowing buttons with shadows"
        echo "  • Smooth gradients"
        echo "  • Inter font"
        ;;
esac
echo ""
echo "Configuration: $CONFIG_DIR"
echo ""
echo "To reload config: Super+Shift+R"
echo "To switch themes: Run this script again"
echo ""
