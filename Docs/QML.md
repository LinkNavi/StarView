# QML Shell Components Tutorial for StarView

This guide teaches you how to create various shell components (panels, docks, launchers, etc.) using Qt/QML and LayerShell.

## Table of Contents
1. [Basic Structure](#basic-structure)
2. [Panel Types](#panel-types)
3. [Creating a Dock](#creating-a-dock)
4. [Creating a Launcher](#creating-a-launcher)
5. [Creating Widgets](#creating-widgets)
6. [IPC Communication](#ipc-communication)
7. [Advanced Styling](#advanced-styling)

---

## 1. Basic Structure

Every QML shell component needs:
- A QML window with LayerShell configuration
- A C++ main.cpp to set up the LayerShell
- A CMakeLists.txt for building
- Optional: IPC handler for compositor communication

### Minimal Template Structure

```
my-shell-app/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── Main.qml
│   └── ipc_handler.h/.cpp (if needed)
└── run.sh
```

---

## 2. Panel Types

### Top Panel (Status Bar)

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.16)
project(my-panel VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)
find_package(LayerShellQt REQUIRED)

qt_add_executable(my-panel
    src/main.cpp
)

qt_add_qml_module(my-panel
    URI MyPanel
    VERSION 1.0
    QML_FILES src/Main.qml
)

target_link_libraries(my-panel PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick
    LayerShellQt::Interface
)
```

**src/main.cpp:**
```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <LayerShellQt/Window>

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORM", "wayland");
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [](QObject *obj, const QUrl &) {
            auto window = qobject_cast<QQuickWindow*>(obj);
            if (!window) return;
            
            auto layer = LayerShellQt::Window::get(window);
            layer->setLayer(LayerShellQt::Window::LayerTop);
            
            // Anchor to top + left + right
            layer->setAnchors(static_cast<LayerShellQt::Window::Anchors>(
                LayerShellQt::Window::AnchorTop | 
                LayerShellQt::Window::AnchorLeft | 
                LayerShellQt::Window::AnchorRight
            ));
            
            layer->setExclusiveZone(40);  // Reserve 40px
            layer->setKeyboardInteractivity(
                LayerShellQt::Window::KeyboardInteractivityNone
            );
            
            window->show();
        });
    
    engine.load(QUrl(QStringLiteral("qrc:/MyPanel/src/Main.qml")));
    return app.exec();
}
```

**src/Main.qml:**
```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: false
    width: 1920
    height: 40
    color: "transparent"
    flags: Qt.FramelessWindowHint
    
    // Background with gradient
    Rectangle {
        anchors.fill: parent
        color: "#1e1e2e"
        opacity: 0.95
        
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#20ffffff" }
                GradientStop { position: 1.0; color: "#00ffffff" }
            }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 20
        
        // Left section
        Text {
            text: "My Panel"
            color: "#cdd6f4"
            font.pixelSize: 14
        }
        
        // Center spacer
        Item { Layout.fillWidth: true }
        
        // Right section - Clock
        Text {
            id: clock
            text: Qt.formatTime(new Date(), "hh:mm")
            color: "#cdd6f4"
            font.pixelSize: 14
            
            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm")
            }
        }
    }
}
```

---

## 3. Creating a Dock

A dock sits at the bottom and shows running applications.

**main.cpp for dock:**
```cpp
auto layer = LayerShellQt::Window::get(window);
layer->setLayer(LayerShellQt::Window::LayerBottom);  // Or LayerTop

// Anchor to bottom only for centered dock
layer->setAnchors(LayerShellQt::Window::AnchorBottom);

// OR anchor to bottom + left + right for full-width dock
layer->setAnchors(static_cast<LayerShellQt::Window::Anchors>(
    LayerShellQt::Window::AnchorBottom | 
    LayerShellQt::Window::AnchorLeft | 
    LayerShellQt::Window::AnchorRight
));

layer->setExclusiveZone(60);  // Reserve 60px
layer->setMargins({0, 0, 10, 0});  // Bottom margin
```

**Dock.qml:**
```qml
ApplicationWindow {
    visible: false
    width: 400  // For centered dock
    height: 60
    color: "transparent"
    flags: Qt.FramelessWindowHint
    
    Rectangle {
        anchors.centerIn: parent
        width: row.width + 20
        height: 50
        radius: 12
        color: "#1e1e2e"
        opacity: 0.9
        
        border.color: "#40ffffff"
        border.width: 1
        
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 10
            
            Repeater {
                model: ["firefox", "kitty", "thunar"]
                
                Rectangle {
                    width: 40
                    height: 40
                    radius: 8
                    color: ma.containsMouse ? "#89b4fa" : "#313244"
                    
                    Behavior on color { ColorAnimation { duration: 200 } }
                    
                    Text {
                        anchors.centerIn: parent
                        text: modelData[0].toUpperCase()
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }
                    
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Launch application
                            Qt.createQmlObject(
                                'import QtQuick; Timer { interval: 1; running: true; repeat: false; onTriggered: Qt.quit() }',
                                parent
                            )
                        }
                    }
                }
            }
        }
    }
}
```

---

## 4. Creating a Launcher

A launcher appears in the center when activated.

**main.cpp for launcher:**
```cpp
auto layer = LayerShellQt::Window::get(window);
layer->setLayer(LayerShellQt::Window::LayerOverlay);  // Top-most

// Center on screen
layer->setAnchors(LayerShellQt::Window::AnchorNone);

layer->setExclusiveZone(-1);  // No exclusive zone
layer->setKeyboardInteractivity(
    LayerShellQt::Window::KeyboardInteractivityExclusive  // Grab keyboard
);
```

**Launcher.qml:**
```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: false
    width: 600
    height: 400
    color: "transparent"
    flags: Qt.FramelessWindowHint
    
    // Background overlay
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        
        MouseArea {
            anchors.fill: parent
            onClicked: Qt.quit()  // Close on background click
        }
    }
    
    // Main launcher window
    Rectangle {
        anchors.centerIn: parent
        width: 500
        height: 350
        radius: 16
        color: "#1e1e2e"
        
        border.color: "#89b4fa"
        border.width: 2
        
        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            // Search field
            TextField {
                id: searchField
                width: parent.width
                height: 40
                placeholderText: "Search applications..."
                
                background: Rectangle {
                    color: "#313244"
                    radius: 8
                    border.color: searchField.activeFocus ? "#89b4fa" : "#45475a"
                    border.width: 2
                }
                
                color: "#cdd6f4"
                font.pixelSize: 14
                
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Escape) {
                        Qt.quit()
                    }
                }
                
                Component.onCompleted: forceActiveFocus()
            }
            
            // Application list
            ListView {
                width: parent.width
                height: parent.height - searchField.height - 15
                clip: true
                spacing: 5
                
                model: ListModel {
                    ListElement { name: "Firefox"; icon: "🦊" }
                    ListElement { name: "Terminal"; icon: "💻" }
                    ListElement { name: "Files"; icon: "📁" }
                    ListElement { name: "Calculator"; icon: "🔢" }
                }
                
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 50
                    radius: 8
                    color: ma.containsMouse ? "#313244" : "transparent"
                    
                    Row {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 15
                        
                        Text {
                            text: model.icon
                            font.pixelSize: 24
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        Text {
                            text: model.name
                            color: "#cdd6f4"
                            font.pixelSize: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Launch app and close launcher
                            console.log("Launch:", model.name)
                            Qt.quit()
                        }
                    }
                }
            }
        }
    }
}
```

---

## 5. Creating Widgets

### Desktop Clock Widget

**main.cpp:**
```cpp
auto layer = LayerShellQt::Window::get(window);
layer->setLayer(LayerShellQt::Window::LayerBottom);  // Below windows

// Position in corner
layer->setAnchors(static_cast<LayerShellQt::Window::Anchors>(
    LayerShellQt::Window::AnchorTop | 
    LayerShellQt::Window::AnchorRight
));

layer->setExclusiveZone(-1);  // Don't reserve space
layer->setMargins({0, 20, 20, 0});  // Top=20, Right=20
layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
```

**ClockWidget.qml:**
```qml
ApplicationWindow {
    visible: false
    width: 200
    height: 200
    color: "transparent"
    flags: Qt.FramelessWindowHint
    
    Rectangle {
        anchors.fill: parent
        radius: 16
        color: "#1e1e2e"
        opacity: 0.8
        
        border.color: "#40ffffff"
        border.width: 1
        
        Column {
            anchors.centerIn: parent
            spacing: 5
            
            Text {
                id: time
                text: Qt.formatTime(new Date(), "hh:mm")
                color: "#cdd6f4"
                font.pixelSize: 48
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                id: date
                text: Qt.formatDate(new Date(), "dddd, MMMM d")
                color: "#6c7086"
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: {
                    var now = new Date()
                    time.text = Qt.formatTime(now, "hh:mm")
                    date.text = Qt.formatDate(now, "dddd, MMMM d")
                }
            }
        }
    }
}
```

### System Monitor Widget

```qml
ApplicationWindow {
    visible: false
    width: 300
    height: 150
    color: "transparent"
    flags: Qt.FramelessWindowHint
    
    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#1e1e2e"
        opacity: 0.9
        
        Column {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10
            
            // CPU Usage
            Row {
                width: parent.width
                spacing: 10
                
                Text {
                    text: "CPU:"
                    color: "#cdd6f4"
                    font.pixelSize: 12
                    width: 50
                }
                
                Rectangle {
                    width: parent.width - 60
                    height: 20
                    radius: 10
                    color: "#313244"
                    
                    Rectangle {
                        width: parent.width * cpuUsage / 100
                        height: parent.height
                        radius: 10
                        color: "#89b4fa"
                        
                        Behavior on width { NumberAnimation { duration: 300 } }
                    }
                }
            }
            
            // Memory Usage
            Row {
                width: parent.width
                spacing: 10
                
                Text {
                    text: "RAM:"
                    color: "#cdd6f4"
                    font.pixelSize: 12
                    width: 50
                }
                
                Rectangle {
                    width: parent.width - 60
                    height: 20
                    radius: 10
                    color: "#313244"
                    
                    Rectangle {
                        width: parent.width * ramUsage / 100
                        height: parent.height
                        radius: 10
                        color: "#a6e3a1"
                        
                        Behavior on width { NumberAnimation { duration: 300 } }
                    }
                }
            }
        }
    }
    
    property real cpuUsage: 0
    property real ramUsage: 0
    
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            // Update with real system data
            cpuUsage = Math.random() * 100
            ramUsage = Math.random() * 100
        }
    }
}
```

---

## 6. IPC Communication

To communicate with StarView compositor:

**ipc_handler.h:**
```cpp
#ifndef IPCHANDLER_H
#define IPCHANDLER_H

#include <QObject>
#include <QLocalSocket>
#include <QTimer>

class IpcHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentWorkspace READ currentWorkspace NOTIFY workspaceChanged)
    
public:
    explicit IpcHandler(QObject *parent = nullptr);
    
    Q_INVOKABLE void connect();
    Q_INVOKABLE void sendCommand(const QString &cmd);
    Q_INVOKABLE void switchWorkspace(int ws);
    
    int currentWorkspace() const { return m_currentWorkspace; }
    
signals:
    void workspaceChanged(int ws);
    void windowChanged(const QString &event);
    void connected();
    void disconnected();
    
private slots:
    void onReadyRead();
    void onError(QLocalSocket::LocalSocketError error);
    
private:
    void sendMessage(uint32_t type, const QByteArray &payload);
    void subscribe();
    
    QLocalSocket *m_socket;
    QByteArray m_readBuffer;
    int m_currentWorkspace = 1;
};

#endif
```

**Usage in QML:**
```qml
ApplicationWindow {
    // ...
    
    Connections {
        target: ipcHandler
        function onWorkspaceChanged(ws) {
            console.log("Workspace:", ws)
            // Update UI
        }
    }
    
    Button {
        text: "Workspace 2"
        onClicked: ipcHandler.switchWorkspace(2)
    }
    
    Component.onCompleted: {
        ipcHandler.connect()
    }
}
```

---

## 7. Advanced Styling

### Glass Morphism Effect

```qml
Rectangle {
    width: 300
    height: 200
    radius: 16
    
    // Blur background (requires compositor support)
    color: "#40ffffff"
    
    border.color: "#60ffffff"
    border.width: 1
    
    // Inner glow
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: parent.radius - 1
        color: "transparent"
        border.color: "#80ffffff"
        border.width: 1
    }
}
```

### Neumorphism Effect

```qml
Rectangle {
    width: 100
    height: 100
    radius: 20
    color: "#1e1e2e"
    
    // Outer shadow (dark)
    layer.enabled: true
    layer.effect: DropShadow {
        color: "#000000"
        radius: 8
        samples: 17
        horizontalOffset: 4
        verticalOffset: 4
    }
    
    // Inner highlight
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: parent.radius - 2
        color: "transparent"
        border.color: "#40ffffff"
        border.width: 1
    }
}
```

### Animations

```qml
Rectangle {
    id: button
    width: 100
    height: 40
    
    // Smooth transitions
    Behavior on scale {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
    
    Behavior on color {
        ColorAnimation { duration: 200 }
    }
    
    MouseArea {
        anchors.fill: parent
        onPressed: button.scale = 0.95
        onReleased: button.scale = 1.0
        onEntered: button.color = "#89b4fa"
        onExited: button.color = "#313244"
    }
    
    // Spring animation
    SequentialAnimation on rotation {
        running: false
        id: wobble
        NumberAnimation { from: 0; to: -5; duration: 100 }
        NumberAnimation { from: -5; to: 5; duration: 100 }
        NumberAnimation { from: 5; to: 0; duration: 100 }
    }
}
```

---

## Building and Running

**run.sh:**
```bash
#!/bin/bash
rm -rf build
mkdir build
cd build
cmake ..
make
./my-shell-app
```

Make executable: `chmod +x run.sh`

Then run: `./run.sh`

---

## Layer Shell Reference

### Layers (z-order, bottom to top):
- `LayerBackground` - Wallpapers
- `LayerBottom` - Widgets, desktop icons
- `LayerTop` - Panels, bars
- `LayerOverlay` - Notifications, launchers

### Anchors:
- `AnchorTop`, `AnchorBottom`, `AnchorLeft`, `AnchorRight`
- Combine with `|` operator

### Keyboard Modes:
- `KeyboardInteractivityNone` - No keyboard input
- `KeyboardInteractivityExclusive` - Grab all keyboard
- `KeyboardInteractivityOnDemand` - Normal focus behavior

### Exclusive Zone:
- Positive value: Reserve that many pixels
- 0: Don't reserve, but can be moved
- -1: Don't reserve, don't move

---

## Example Projects

Check out these example projects in the repository:
- `Bar/` - Full-featured status bar with workspace buttons
- `examples/dock/` - Application dock
- `examples/launcher/` - Application launcher
- `examples/clock-widget/` - Desktop clock widget

Happy coding!
