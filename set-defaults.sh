#!/bin/bash
# StarView Configuration Setup Script

CONFIG_DIR="$HOME/.config/starview"
KEYBINDS_DIR="$CONFIG_DIR/keybinds"

echo "Setting up StarView configuration..."

# Create directory structure
mkdir -p "$CONFIG_DIR"
mkdir -p "$KEYBINDS_DIR"

# Create main configuration file
cat > "$CONFIG_DIR/starview.toml" << 'EOF'
# StarView Main Configuration
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

cat > "$CONFIG_DIR/gestures.toml" << 'EOF'
# Gesture Configuration

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

[[gesture_touchpad]]
fingers = 3
direction = "up"
action = "maximize"

[[gesture_touchpad]]
fingers = 3
direction = "down"
action = "minimize"

# Mouse Gestures - Alt+Middle Click drag for window management
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

# Super+Right Click for quick app launcher
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

# Alt+Right Click for workspace switching
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

# Create general.toml
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

# Create decoration.toml
cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 32
button_size = 16
button_spacing = 10
corner_radius = 10

# Catppuccin Mocha theme
bg_color = "#1e1e2e"
bg_color_inactive = "#313244"
title_color = "#cdd6f4"
title_color_inactive = "#6c7086"

# Traffic light style buttons
close_color = "#f38ba8"
maximize_color = "#a6e3a1"
minimize_color = "#f9e2af"

font = "sans-serif"
font_size = 11
buttons_left = false
EOF

# Create animation.toml
cat > "$CONFIG_DIR/animation.toml" << 'EOF'
[animation]
enabled = true
duration = 180

window_open = "zoom"
window_close = "fade"
window_move = "none"
window_resize = "none"
workspace_switch = "slide"

curve = "ease_out"
fade_min = 0.0
zoom_min = 0.85
EOF

# Create tiling.toml
cat > "$CONFIG_DIR/tiling.toml" << 'EOF'
[tiling]
# Dwindle layout settings
master_ratio = 0.55
master_count = 1
EOF

# Create keybind files
cat > "$KEYBINDS_DIR/apps.toml" << 'EOF'
[keybinds]
# Terminal
"Super+Return" = "spawn kitty"
"Super+Shift+Return" = "spawn alacritty"

# Launcher
"Super+d" = "spawn wofi --show drun"
"Super+r" = "spawn wofi --show run"

# Common apps
"Super+b" = "spawn firefox"
"Super+e" = "spawn thunar"
"Super+c" = "spawn code"
EOF

cat > "$KEYBINDS_DIR/windows.toml" << 'EOF'
[keybinds]
# Window actions
"Super+q" = "close"
"Super+f" = "fullscreen"
"Super+Space" = "toggle_floating"
"Super+m" = "minimize"
"Super+Shift+m" = "maximize"

# Snapping
"Super+Left" = "snap_left"
"Super+Right" = "snap_right"
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
EOF

cat > "$KEYBINDS_DIR/move.toml" << 'EOF'
[keybinds]
# Move windows in tiling layout (swap)
"Super+Shift+h" = "swap_left"
"Super+Shift+j" = "swap_down"
"Super+Shift+k" = "swap_up"
"Super+Shift+l" = "swap_right"

# Move in floating mode
"Super+Ctrl+h" = "move left"
"Super+Ctrl+j" = "move down"
"Super+Ctrl+k" = "move up"
"Super+Ctrl+l" = "move right"

# Preselect (choose where next window opens)
"Super+Alt+h" = "preselect_left"
"Super+Alt+j" = "preselect_down"
"Super+Alt+k" = "preselect_up"
"Super+Alt+l" = "preselect_right"
EOF

cat > "$KEYBINDS_DIR/resize.toml" << 'EOF'
[keybinds]
# Resize windows (floating mode)
"Super+Ctrl+Shift+h" = "resize_shrink_width"
"Super+Ctrl+Shift+l" = "resize_grow_width"
"Super+Ctrl+Shift+k" = "resize_shrink_height"
"Super+Ctrl+Shift+j" = "resize_grow_height"
EOF

cat > "$KEYBINDS_DIR/workspaces.toml" << 'EOF'
[keybinds]
# Switch workspace
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

# Tiling adjustments
"Super+i" = "inc_master_count"
"Super+o" = "dec_master_count"
"Super+minus" = "dec_master_ratio"
"Super+equal" = "inc_master_ratio"
EOF

cat > "$KEYBINDS_DIR/system.toml" << 'EOF'
[keybinds]
# System
"Super+Shift+r" = "reload_config"
"Super+Shift+e" = "exit"

# Screenshots
"Print" = "spawn grim ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
"Shift+Print" = "spawn grim -g \"$(slurp)\" ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"
"Super+Shift+s" = "spawn grim -g \"$(slurp)\" ~/Pictures/screenshot-$(date +%Y%m%d-%H%M%S).png"

# Screen lock
"Super+Escape" = "spawn swaylock"
EOF

# Create rules.toml
cat > "$CONFIG_DIR/rules.toml" << 'EOF'
# Window Rules

# Browser on workspace 2
[[rules]]
app_id = "firefox"
workspace = 2

[[rules]]
app_id = "chromium"
workspace = 2

# Picture-in-picture always floating
[[rules]]
title = "*Picture-in-Picture*"
floating = true

[[rules]]
title = "*PiP*"
floating = true

# Media players floating
[[rules]]
app_id = "mpv"
floating = true
width = 1280
height = 720

# System utilities floating
[[rules]]
app_id = "pavucontrol"
floating = true
width = 800
height = 600

[[rules]]
app_id = "blueman-manager"
floating = true

[[rules]]
app_id = "nm-connection-editor"
floating = true

# File pickers always floating
[[rules]]
title = "*Open File*"
floating = true

[[rules]]
title = "*Save As*"
floating = true

# Dialogs floating
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

# Discord
[[rules]]
app_id = "discord"
workspace = 8

# Floating calculator
[[rules]]
app_id = "gnome-calculator"
floating = true
width = 400
height = 600

[[rules]]
app_id = "org.kde.kcalc"
floating = true
EOF

echo ""
echo "✓ StarView configuration created at $CONFIG_DIR"
echo ""
echo "Configuration includes:"
echo "  • Hyprland-style dwindle tiling layout"
echo "  • Mouse gestures (Alt+Middle, Super+Right, Alt+Right)"
echo "  • Touchpad gestures (3-finger swipes)"
echo "  • Window swapping (Super+Shift+h/j/k/l)"
echo "  • Preselect (Super+Alt+h/j/k/l)"
echo "  • Vim-style navigation"
echo "  • Catppuccin Mocha theme"
echo ""
echo "Key bindings:"
echo "  Super+Return        - Terminal"
echo "  Super+d             - App launcher"
echo "  Super+q             - Close window"
echo "  Super+h/j/k/l       - Focus window"
echo "  Super+Shift+h/j/k/l - Swap windows"
echo "  Super+Alt+h/j/k/l   - Preselect window position"
echo "  Super+1-9           - Switch workspace"
echo "  Alt+Left Click      - Move/swap window"
echo "  Alt+Right Click     - Resize (floating)"
echo "  Alt+Middle drag     - Window actions"
echo ""
echo "Edit files in $CONFIG_DIR to customize"
echo ""
