#ifndef IPC_H
#define IPC_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <wayland-server-core.h>

// Forward declarations
struct server;
struct toplevel;

// i3 IPC magic string
#define IPC_MAGIC "i3-ipc"
#define IPC_MAGIC_LEN 6

// Message types (i3-compatible)
enum ipc_command_type {
    IPC_COMMAND = 0,
    IPC_GET_WORKSPACES = 1,
    IPC_SUBSCRIBE = 2,
    IPC_GET_OUTPUTS = 3,
    IPC_GET_TREE = 4,
    IPC_GET_MARKS = 5,
    IPC_GET_BAR_CONFIG = 6,
    IPC_GET_VERSION = 7,
    IPC_GET_BINDING_MODES = 8,
    IPC_GET_CONFIG = 9,
    IPC_SEND_TICK = 10,
    IPC_SYNC = 11,
    IPC_GET_BINDING_STATE = 12,
    IPC_GET_INPUTS = 100,
    IPC_GET_SEATS = 101,
};

// Event types (bit flags for subscription)
enum ipc_event_type {
    IPC_EVENT_WORKSPACE = (1 << 0),
    IPC_EVENT_OUTPUT = (1 << 1),
    IPC_EVENT_MODE = (1 << 2),
    IPC_EVENT_WINDOW = (1 << 3),
    IPC_EVENT_BARCONFIG_UPDATE = (1 << 4),
    IPC_EVENT_BINDING = (1 << 5),
    IPC_EVENT_SHUTDOWN = (1 << 6),
    IPC_EVENT_TICK = (1 << 7),
};

// Reply types (0x80000000 | command type)
#define IPC_EVENT_MASK 0x80000000

// IPC server functions
int ipc_init(struct server *server);
void ipc_finish(struct server *server);
void ipc_event_workspace(struct server *server);
void ipc_event_window(struct server *server, struct toplevel *toplevel, const char *change);
void ipc_event_mode(struct server *server);

#endif
