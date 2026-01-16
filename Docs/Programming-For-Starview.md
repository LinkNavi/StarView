# StarView Shell App Development Guide

## Overview

StarView shell apps are Wayland applications that use the **layer-shell protocol** to appear as part of the desktop environment (panels, docks, notifications, etc.) rather than as regular windows.

## Layer Shell Protocol

StarView supports the `wlr-layer-shell-unstable-v1` protocol with 4 layers:
- **BACKGROUND** (0) - Wallpapers
- **BOTTOM** (1) - Desktop widgets, docks
- **TOP** (2) - Panels, bars
- **OVERLAY** (3) - Notifications, OSD

## Basic Shell App Structure

### 1. GTK4 Layer Shell (Recommended)

```c
// panel.c - Example status bar
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    
    // Initialize layer shell
    gtk_layer_init_for_window(GTK_WINDOW(window));
    
    // Set layer (TOP for panels)
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    
    // Anchor to edges (top of screen)
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    
    // Set margins
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    
    // Exclusive zone (reserve space, don't overlap with windows)
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 30);
    
    // Keyboard interactivity
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), 
        GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    
    // Build UI
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    
    GtkWidget *label = gtk_label_new("StarView Panel");
    gtk_box_append(GTK_BOX(box), label);
    
    GtkWidget *clock = gtk_label_new("");
    gtk_widget_set_hexpand(clock, TRUE);
    gtk_widget_set_halign(clock, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), clock);
    
    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new(
        "com.starview.panel", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
```

**Build with:**
```bash
gcc panel.c -o starview-panel \
    $(pkg-config --cflags --libs gtk4 gtk4-layer-shell-0)
```

### 2. Qt Layer Shell

```cpp
// dock.cpp - Application dock
#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <LayerShellQt/Window>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QWidget *window = new QWidget();
    window->setWindowTitle("StarView Dock");
    
    // Setup layer shell
    auto layerShell = LayerShellQt::Window::get(window->windowHandle());
    layerShell->setLayer(LayerShellQt::Window::LayerBottom);
    layerShell->setAnchors(LayerShellQt::Window::AnchorBottom | 
                           LayerShellQt::Window::AnchorLeft | 
                           LayerShellQt::Window::AnchorRight);
    layerShell->setExclusiveZone(60);
    layerShell->setMargins({0, 0, 10, 0});
    
    // Build UI
    QHBoxLayout *layout = new QHBoxLayout(window);
    layout->addWidget(new QPushButton("App 1"));
    layout->addWidget(new QPushButton("App 2"));
    layout->addWidget(new QPushButton("App 3"));
    
    window->show();
    return app.exec();
}
```

## Shell App Types

### Status Bar / Panel

**Characteristics:**
- Layer: TOP
- Anchors: Top + Left + Right (or Bottom)
- Exclusive zone: Bar height (30-40px)
- Keyboard: NONE or ON_DEMAND

**Features to implement:**
- Clock/date
- System tray
- Workspace indicators
- Volume/brightness
- Battery status
- Network status

### Application Dock

**Characteristics:**
- Layer: BOTTOM or TOP
- Anchors: Bottom + Left + Right (or just Bottom for centered)
- Exclusive zone: Dock height
- Keyboard: NONE

**Features:**
- Pinned applications
- Running app indicators
- Click to launch/focus
- Drag and drop reordering

### Notification Daemon

**Characteristics:**
- Layer: OVERLAY
- Anchors: Top + Right (or desired corner)
- Exclusive zone: -1 (no exclusive)
- Keyboard: NONE

**Protocol:** Implement freedesktop.org notifications spec via D-Bus

```c
// Listen for org.freedesktop.Notifications on session bus
// Methods: Notify, CloseNotification, GetCapabilities, GetServerInformation
```

### Launcher / Application Menu

**Characteristics:**
- Layer: OVERLAY
- Anchors: Center or specific position
- Exclusive zone: -1
- Keyboard: EXCLUSIVE (capture all keyboard)

**Best practice:** Use `wofi`, `rofi`, or build custom with fuzzy search

### Wallpaper Manager

**Characteristics:**
- Layer: BACKGROUND
- Anchors: All edges
- Exclusive zone: -1
- Keyboard: NONE

**Simple implementation:**
```bash
# Use swaybg
swaybg -i ~/wallpaper.png -m fill
```

### Screen Lock

**Characteristics:**
- Layer: OVERLAY
- Anchors: All edges
- Exclusive zone: -1
- Keyboard: EXCLUSIVE

**Security:** Must handle PAM authentication properly

## Communication with Compositor

### IPC Protocol (Recommended)

Create a socket-based IPC:

```c
// In compositor: listen on /run/user/$(id -u)/starview.sock
// Protocol: JSON messages

// Example messages:
{"type": "get_workspaces"}
{"type": "subscribe", "events": ["workspace", "window"]}
{"type": "command", "cmd": "workspace 2"}

// Events sent to subscribers:
{"event": "workspace", "current": 2, "old": 1}
{"event": "window", "change": "focus", "container": {...}}
```

### Using the IPC in Shell Apps

```python
#!/usr/bin/env python3
import socket
import json

def starview_ipc(msg):
    sock = socket.socket(socket.AF_UNIX)
    sock.connect(f"/run/user/{os.getuid()}/starview.sock")
    sock.send(json.dumps(msg).encode() + b'\n')
    response = sock.recv(4096)
    sock.close()
    return json.loads(response)

# Get workspace info
workspaces = starview_ipc({"type": "get_workspaces"})
print(f"Current workspace: {workspaces['current']}")
```

## Best Practices

### 1. Resource Usage
- Keep shell apps lightweight
- Use efficient update intervals (1s for clock is fine)
- Batch D-Bus/IPC requests

### 2. Styling
- Match compositor theme (read from StarView config)
- Use CSS for GTK apps:
```css
window {
    background-color: #1e1e2e;
    color: #cdd6f4;
}
```

### 3. Positioning
- Use anchors, not absolute positioning
- Respect exclusive zones
- Don't overlap other shell components

### 4. Keyboard Handling
- NONE: No keyboard input (panels, docks)
- ON_DEMAND: Grab keyboard when needed (launcher)
- EXCLUSIVE: Always grab keyboard (screen lock)

### 5. Error Handling
- Gracefully handle compositor restart
- Reconnect to IPC on disconnect
- Log errors to syslog or file

## Example: Complete Workspace Indicator

```c
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <json-c/json.h>
#include <sys/socket.h>
#include <sys/un.h>

GtkWidget *ws_labels[10];

void update_workspaces(int current) {
    for (int i = 0; i < 10; i++) {
        if (i == current - 1) {
            gtk_widget_add_css_class(ws_labels[i], "active");
        } else {
            gtk_widget_remove_css_class(ws_labels[i], "active");
        }
    }
}

gboolean poll_workspaces(gpointer data) {
    // Query compositor for workspace info
    // Update UI accordingly
    return G_SOURCE_CONTINUE;
}

static void on_workspace_click(GtkButton *button, gpointer data) {
    int ws = GPOINTER_TO_INT(data);
    // Send IPC command to switch workspace
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, 10);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 10);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    for (int i = 0; i < 10; i++) {
        char label[8];
        snprintf(label, sizeof(label), "%d", i + 1);
        GtkWidget *btn = gtk_button_new_with_label(label);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_workspace_click), 
                        GINT_TO_POINTER(i + 1));
        gtk_box_append(GTK_BOX(box), btn);
        ws_labels[i] = btn;
    }
    
    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
    
    g_timeout_add_seconds(1, poll_workspaces, NULL);
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new(
        "com.starview.workspace", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
```

## Testing Your Shell App

1. **Start StarView compositor**
2. **Launch your shell app**: `./my-shell-app`
3. **Check layer placement**: `WAYLAND_DEBUG=1 ./my-shell-app`
4. **Monitor IPC**: `socat - UNIX-CONNECT:/run/user/$(id -u)/starview.sock`

## Recommended Shell App Stack

**Minimum viable setup:**
- `waybar` - Status bar
- `wofi` - Application launcher
- `swaybg` - Wallpaper
- `mako` - Notifications

**Full custom stack:**
- Custom status bar (GTK4 + layer-shell)
- Custom dock (Qt + LayerShellQt)
- Custom launcher (GTK4 + layer-shell + fuzzy search)
- `swaybg` or custom wallpaper manager
- Custom notification daemon (D-Bus + GTK4)

## Resources

- **wlr-layer-shell protocol**: https://wayland.app/protocols/wlr-layer-shell-unstable-v1
- **gtk4-layer-shell**: https://github.com/wmww/gtk4-layer-shell
- **LayerShellQt**: https://invent.kde.org/plasma/layer-shell-qt
- **Waybar source**: Great reference for status bar implementation
- **Mako source**: Reference for notification daemon
