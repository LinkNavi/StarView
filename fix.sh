#!/bin/bash
# Diagnostic script to check decoration.toml values

echo "=== Checking decoration.toml configuration ==="
echo ""

CONFIG_FILE="$HOME/.config/starview/decoration.toml"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "ERROR: $CONFIG_FILE not found!"
    exit 1
fi

echo "File location: $CONFIG_FILE"
echo ""
echo "=== Current values ==="
grep -E "^(height|button_size|corner_radius|bg_color|title_color|btn_close_color|btn_max_color|btn_min_color|buttons_left)" "$CONFIG_FILE"
echo ""
echo "=== Checking for problematic lines ==="

# Check if button_size has quotes or weird formatting
BUTTON_SIZE=$(grep "^button_size" "$CONFIG_FILE")
echo "button_size line: $BUTTON_SIZE"

if echo "$BUTTON_SIZE" | grep -q '"'; then
    echo "  ⚠️  WARNING: button_size has quotes (should be a plain number)"
fi

# Check each color value
for color_var in bg_color bg_color_inactive title_color title_color_inactive btn_close_color btn_close_hover btn_max_color btn_max_hover btn_min_color btn_min_hover; do
    LINE=$(grep "^$color_var" "$CONFIG_FILE" 2>/dev/null)
    if [ -n "$LINE" ]; then
        VALUE=$(echo "$LINE" | cut -d'=' -f2 | tr -d ' ')
        # Check if it starts with 0x
        if ! echo "$VALUE" | grep -q "^0x"; then
            echo "  ⚠️  WARNING: $color_var = $VALUE (should start with 0x)"
        fi
        # Check length (should be 0xRRGGBBAA = 10 chars)
        LEN=${#VALUE}
        if [ $LEN -ne 10 ]; then
            echo "  ⚠️  WARNING: $color_var = $VALUE (length $LEN, should be 10 chars like 0xRRGGBBAA)"
        fi
    fi
done

echo ""
echo "=== To fix issues ==="
echo "Run: ./set-defaults.sh"
echo "Then select your theme again (option 4 for GNOME)"
