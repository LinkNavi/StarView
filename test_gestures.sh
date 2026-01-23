#!/bin/bash

# Run compositor with logging
echo "Starting compositor with gesture logging..."
echo "Watch for [GESTURE] messages when you:"
echo "1. Hold Alt + Middle Click"
echo "2. Drag 50+ pixels"
echo "3. Release"
echo ""

# Run and show stderr
./build/compositor 2>&1 | grep --line-buffered -E '\[GESTURE\]|gesture_mouse_count|Failed|Error'
