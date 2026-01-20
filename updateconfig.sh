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
    "keybinds/",
    "rules.toml",
]
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

# Create decoration.toml
cat > "$CONFIG_DIR/decoration.toml" << 'EOF'
[decoration]
enabled = true
height = 30
button_size = 24
button_spacing = 8
corner_radius = 8

# Catppuccin Mocha theme
bg_color = "#1e1e2e"
bg_color_inactive = "#313244"
title_color = "#cdd6f4"
title_color_inactive = "#6c7086"

close_color = "#f38ba8"
close_hover = "#eba0ac"
maximize_color = "#a6e3a1"
maximize_hover = "#94e2d5"
minimize_color = "#f9e2af"
minimize_hover = "#f5c2e7"

font = "sans"
font_size = 12
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
floating =
