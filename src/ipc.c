#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "ipc.h"
#include "core.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <json-c/json.h>

// IPC client structure (internal to ipc.c)
struct ipc_client {
    struct wl_list link;
    int fd;
    struct wl_event_source *event_source;
    struct server *server;
    
    uint32_t subscribed_events;
    
    char *read_buffer;
    size_t read_buffer_size;
    size_t read_buffer_len;
};

static struct wl_list ipc_clients;
static int ipc_socket = -1;
static struct wl_event_source *ipc_event_source = NULL;

// IPC message header
struct ipc_header {
    char magic[6];  // "i3-ipc"
    uint32_t size;
    uint32_t type;
} __attribute__((packed));

static void ipc_client_handle_command(struct ipc_client *client, uint32_t type, const char *payload, uint32_t size);

static int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void ipc_send_response(struct ipc_client *client, uint32_t type, const char *payload) {
    size_t payload_len = payload ? strlen(payload) : 0;
    size_t total_len = sizeof(struct ipc_header) + payload_len;
    
    char *buffer = malloc(total_len);
    if (!buffer) return;
    
    struct ipc_header *header = (struct ipc_header *)buffer;
    memcpy(header->magic, IPC_MAGIC, IPC_MAGIC_LEN);
    header->size = payload_len;
    header->type = type;
    
    if (payload_len > 0) {
        memcpy(buffer + sizeof(struct ipc_header), payload, payload_len);
    }
    
    ssize_t written = write(client->fd, buffer, total_len);
    (void)written;
    
    free(buffer);
}

static void ipc_send_event(struct ipc_client *client, uint32_t event_type, const char *payload) {
    if (!(client->subscribed_events & event_type)) return;
    
    uint32_t type = IPC_EVENT_MASK | event_type;
    ipc_send_response(client, type, payload);
}

// JSON builders
char *ipc_json_describe_workspace(struct server *server, int workspace) {
    struct json_object *obj = json_object_new_object();
    
    json_object_object_add(obj, "num", json_object_new_int(workspace));
    json_object_object_add(obj, "name", json_object_new_string(
        workspace == 1 ? "1" :
        workspace == 2 ? "2" :
        workspace == 3 ? "3" :
        workspace == 4 ? "4" : "5"));
    
    json_object_object_add(obj, "visible", 
        json_object_new_boolean(workspace == server->current_workspace));
    json_object_object_add(obj, "focused", 
        json_object_new_boolean(workspace == server->current_workspace));
    json_object_object_add(obj, "urgent", json_object_new_boolean(false));
    
    struct json_object *rect = json_object_new_object();
    json_object_object_add(rect, "x", json_object_new_int(0));
    json_object_object_add(rect, "y", json_object_new_int(0));
    json_object_object_add(rect, "width", json_object_new_int(1920));
    json_object_object_add(rect, "height", json_object_new_int(1080));
    json_object_object_add(obj, "rect", rect);
    
    json_object_object_add(obj, "output", json_object_new_string("HDMI-A-1"));
    
    const char *json_str = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    char *result = strdup(json_str);
    json_object_put(obj);
    
    return result;
}

char *ipc_json_describe_workspaces(struct server *server) {
    struct json_object *arr = json_object_new_array();
    
    for (int i = 1; i <= MAX_WORKSPACES; i++) {
        struct json_object *ws = json_object_new_object();
        
        json_object_object_add(ws, "num", json_object_new_int(i));
        
        char name[8];
        snprintf(name, sizeof(name), "%d", i);
        json_object_object_add(ws, "name", json_object_new_string(name));
        
        json_object_object_add(ws, "visible", 
            json_object_new_boolean(i == server->current_workspace));
        json_object_object_add(ws, "focused", 
            json_object_new_boolean(i == server->current_workspace));
        json_object_object_add(ws, "urgent", json_object_new_boolean(false));
        
        struct json_object *rect = json_object_new_object();
        json_object_object_add(rect, "x", json_object_new_int(0));
        json_object_object_add(rect, "y", json_object_new_int(0));
        json_object_object_add(rect, "width", json_object_new_int(1920));
        json_object_object_add(rect, "height", json_object_new_int(1080));
        json_object_object_add(ws, "rect", rect);
        
        json_object_object_add(ws, "output", json_object_new_string("HDMI-A-1"));
        
        json_object_array_add(arr, ws);
    }
    
    const char *json_str = json_object_to_json_string_ext(arr, JSON_C_TO_STRING_PLAIN);
    char *result = strdup(json_str);
    json_object_put(arr);
    
    return result;
}

char *ipc_json_describe_outputs(struct server *server) {
    struct json_object *arr = json_object_new_array();
    
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        struct json_object *obj = json_object_new_object();
        
        json_object_object_add(obj, "name", 
            json_object_new_string(output->wlr_output->name));
        json_object_object_add(obj, "active", json_object_new_boolean(true));
        json_object_object_add(obj, "primary", json_object_new_boolean(true));
        
        struct json_object *rect = json_object_new_object();
        json_object_object_add(rect, "x", json_object_new_int(0));
        json_object_object_add(rect, "y", json_object_new_int(0));
        json_object_object_add(rect, "width", 
            json_object_new_int(output->wlr_output->width));
        json_object_object_add(rect, "height", 
            json_object_new_int(output->wlr_output->height));
        json_object_object_add(obj, "rect", rect);
        
        json_object_object_add(obj, "current_workspace", 
            json_object_new_string("1"));
        
        json_object_array_add(arr, obj);
    }
    
    const char *json_str = json_object_to_json_string_ext(arr, JSON_C_TO_STRING_PLAIN);
    char *result = strdup(json_str);
    json_object_put(arr);
    
    return result;
}

char *ipc_json_describe_tree(struct server *server) {
    struct json_object *root = json_object_new_object();
    
    json_object_object_add(root, "id", json_object_new_int(1));
    json_object_object_add(root, "type", json_object_new_string("root"));
    json_object_object_add(root, "orientation", json_object_new_string("none"));
    json_object_object_add(root, "focused", json_object_new_boolean(false));
    
    struct json_object *nodes = json_object_new_array();
    json_object_object_add(root, "nodes", nodes);
    
    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
    char *result = strdup(json_str);
    json_object_put(root);
    
    return result;
}

char *ipc_json_describe_version(void) {
    struct json_object *obj = json_object_new_object();
    
    json_object_object_add(obj, "human_readable", 
        json_object_new_string("starview 0.1.0"));
    json_object_object_add(obj, "variant", json_object_new_string("starview"));
    json_object_object_add(obj, "major", json_object_new_int(0));
    json_object_object_add(obj, "minor", json_object_new_int(1));
    json_object_object_add(obj, "patch", json_object_new_int(0));
    
    const char *json_str = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    char *result = strdup(json_str);
    json_object_put(obj);
    
    return result;
}

static void ipc_handle_command(struct ipc_client *client, const char *payload) {
    if (!payload) return;
    
    // Parse command and execute
    if (strncmp(payload, "workspace ", 10) == 0) {
        int ws = atoi(payload + 10);
        if (ws >= 1 && ws <= MAX_WORKSPACES) {
            workspace_show(client->server, ws);
        }
    } else if (strcmp(payload, "reload") == 0) {
        config_reload();
        arrange_windows(client->server);
    } else if (strcmp(payload, "exit") == 0) {
        wl_display_terminate(client->server->display);
    }
    
    struct json_object *arr = json_object_new_array();
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(true));
    json_object_array_add(arr, result);
    
    const char *json_str = json_object_to_json_string_ext(arr, JSON_C_TO_STRING_PLAIN);
    ipc_send_response(client, IPC_COMMAND, json_str);
    json_object_put(arr);
}

static void ipc_handle_subscribe(struct ipc_client *client, const char *payload) {
    if (!payload) return;
    
    struct json_object *obj = json_tokener_parse(payload);
    if (!obj) return;
    
    size_t len = json_object_array_length(obj);
    for (size_t i = 0; i < len; i++) {
        struct json_object *item = json_object_array_get_idx(obj, i);
        const char *event = json_object_get_string(item);
        
        if (strcmp(event, "workspace") == 0) {
            client->subscribed_events |= IPC_EVENT_WORKSPACE;
        } else if (strcmp(event, "window") == 0) {
            client->subscribed_events |= IPC_EVENT_WINDOW;
        } else if (strcmp(event, "mode") == 0) {
            client->subscribed_events |= IPC_EVENT_MODE;
        }
    }
    
    struct json_object *result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(true));
    
    const char *json_str = json_object_to_json_string_ext(result, JSON_C_TO_STRING_PLAIN);
    ipc_send_response(client, IPC_SUBSCRIBE, json_str);
    json_object_put(result);
    json_object_put(obj);
}

static void ipc_client_handle_command(struct ipc_client *client, uint32_t type, const char *payload, uint32_t size) {
    (void)size;
    
    switch (type) {
    case IPC_COMMAND: {
        ipc_handle_command(client, payload);
        break;
    }
    
    case IPC_GET_WORKSPACES: {
        char *json = ipc_json_describe_workspaces(client->server);
        ipc_send_response(client, IPC_GET_WORKSPACES, json);
        free(json);
        break;
    }
    
    case IPC_GET_OUTPUTS: {
        char *json = ipc_json_describe_outputs(client->server);
        ipc_send_response(client, IPC_GET_OUTPUTS, json);
        free(json);
        break;
    }
    
    case IPC_GET_TREE: {
        char *json = ipc_json_describe_tree(client->server);
        ipc_send_response(client, IPC_GET_TREE, json);
        free(json);
        break;
    }
    
    case IPC_GET_VERSION: {
        char *json = ipc_json_describe_version();
        ipc_send_response(client, IPC_GET_VERSION, json);
        free(json);
        break;
    }
    
    case IPC_SUBSCRIBE: {
        ipc_handle_subscribe(client, payload);
        break;
    }
    
    default:
        break;
    }
}

void ipc_client_disconnect(struct ipc_client *client) {
    wl_list_remove(&client->link);
    wl_event_source_remove(client->event_source);
    close(client->fd);
    free(client->read_buffer);
    free(client);
}

static int ipc_client_handle_readable(int fd, uint32_t mask, void *data) {
    struct ipc_client *client = data;
    (void)mask;
    
    if (client->read_buffer_size == 0) {
        client->read_buffer_size = 4096;
        client->read_buffer = malloc(client->read_buffer_size);
    }
    
    ssize_t n = read(fd, client->read_buffer + client->read_buffer_len,
                     client->read_buffer_size - client->read_buffer_len);
    
    if (n <= 0) {
        ipc_client_disconnect(client);
        return 0;
    }
    
    client->read_buffer_len += n;
    
    while (client->read_buffer_len >= sizeof(struct ipc_header)) {
        struct ipc_header *header = (struct ipc_header *)client->read_buffer;
        
        if (memcmp(header->magic, IPC_MAGIC, IPC_MAGIC_LEN) != 0) {
            ipc_client_disconnect(client);
            return 0;
        }
        
        size_t total_len = sizeof(struct ipc_header) + header->size;
        if (client->read_buffer_len < total_len) {
            break;
        }
        
        char *payload = NULL;
        if (header->size > 0) {
            payload = malloc(header->size + 1);
            memcpy(payload, client->read_buffer + sizeof(struct ipc_header), header->size);
            payload[header->size] = '\0';
        }
        
        ipc_client_handle_command(client, header->type, payload, header->size);
        free(payload);
        
        memmove(client->read_buffer, client->read_buffer + total_len,
                client->read_buffer_len - total_len);
        client->read_buffer_len -= total_len;
    }
    
    return 0;
}

static int ipc_handle_connection(int fd, uint32_t mask, void *data) {
    struct server *server = data;
    (void)mask;
    
    int client_fd = accept(fd, NULL, NULL);
    if (client_fd == -1) {
        return 0;
    }
    
    set_nonblock(client_fd);
    
    struct ipc_client *client = calloc(1, sizeof(*client));
    client->fd = client_fd;
    client->server = server;
    client->subscribed_events = 0;
    
    struct wl_event_loop *loop = wl_display_get_event_loop(server->display);
    client->event_source = wl_event_loop_add_fd(loop, client_fd,
        WL_EVENT_READABLE, ipc_client_handle_readable, client);
    
    wl_list_insert(&ipc_clients, &client->link);
    
    return 0;
}

int ipc_init(struct server *server) {
    wl_list_init(&ipc_clients);
    
    const char *socket_path = getenv("SWAYSOCK");
    if (!socket_path) {
        socket_path = "/tmp/starview-ipc.sock";
    }
    
    unlink(socket_path);
    
    ipc_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ipc_socket == -1) {
        return -1;
    }
    
    set_nonblock(ipc_socket);
    
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (bind(ipc_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(ipc_socket);
        return -1;
    }
    
    if (listen(ipc_socket, 8) == -1) {
        close(ipc_socket);
        return -1;
    }
    
    struct wl_event_loop *loop = wl_display_get_event_loop(server->display);
    ipc_event_source = wl_event_loop_add_fd(loop, ipc_socket,
        WL_EVENT_READABLE, ipc_handle_connection, server);
    
    setenv("SWAYSOCK", socket_path, 1);
    printf("IPC socket: %s\n", socket_path);
    
    return 0;
}

void ipc_finish(struct server *server) {
    (void)server;
    
    if (ipc_event_source) {
        wl_event_source_remove(ipc_event_source);
    }
    
    if (ipc_socket != -1) {
        close(ipc_socket);
    }
    
    struct ipc_client *client, *tmp;
    wl_list_for_each_safe(client, tmp, &ipc_clients, link) {
        ipc_client_disconnect(client);
    }
}

void ipc_event_workspace(struct server *server) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "change", json_object_new_string("focus"));
    
    struct json_object *current = json_object_new_object();
    json_object_object_add(current, "num", json_object_new_int(server->current_workspace));
    json_object_object_add(obj, "current", current);
    
    const char *json_str = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    
    struct ipc_client *client;
    wl_list_for_each(client, &ipc_clients, link) {
        ipc_send_event(client, IPC_EVENT_WORKSPACE, json_str);
    }
    
    json_object_put(obj);
}

void ipc_event_window(struct server *server, struct toplevel *toplevel, const char *change) {
    (void)server;
    (void)toplevel;
    
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "change", json_object_new_string(change));
    
    const char *json_str = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    
    struct ipc_client *client;
    wl_list_for_each(client, &ipc_clients, link) {
        ipc_send_event(client, IPC_EVENT_WINDOW, json_str);
    }
    
    json_object_put(obj);
}

void ipc_event_mode(struct server *server) {
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "change", 
        json_object_new_string(server->mode == MODE_TILING ? "tiling" : "floating"));
    
    const char *json_str = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    
    struct ipc_client *client;
    wl_list_for_each(client, &ipc_clients, link) {
        ipc_send_event(client, IPC_EVENT_MODE, json_str);
    }
    
    json_object_put(obj);
}
