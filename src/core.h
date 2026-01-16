#ifndef CORE_H
#define CORE_H

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

enum cursor_mode { CURSOR_NORMAL, CURSOR_MOVE, CURSOR_RESIZE };

struct layer_surface {
    struct server *server;
    struct wlr_layer_surface_v1 *wlr_layer;
    struct wlr_scene_tree *scene_tree;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
};

struct cursor_state {
    struct toplevel *toplevel;
    double grab_x, grab_y;
    struct wlr_box grab_box;
    enum cursor_mode mode;
    uint32_t resize_edges;
};

extern struct cursor_state cursor_state;

struct server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_seat *seat;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    
    struct wlr_layer_shell_v1 *layer_shell;
    struct wl_listener new_layer_surface;
    
    struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
    struct wl_listener new_xdg_decoration;
    
    struct wl_listener new_output;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_xdg_popup;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_abs;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    
    // Scene layers (bottom to top)
    struct wlr_scene_tree *layer_bg;
    struct wlr_scene_tree *layer_bottom;
    struct wlr_scene_tree *layer_windows;
    struct wlr_scene_tree *layer_top;
    struct wlr_scene_tree *layer_overlay;
    
    struct wl_list outputs;
    struct wl_list toplevels;
    struct wl_list keyboards;
    
    // Window management mode
    enum window_mode mode;
    int current_workspace;
};

struct output {
    struct wl_list link;
    struct server *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
};

struct toplevel {
    struct wl_list link;
    struct server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener request_fullscreen;
    
    // Window state
    bool floating;          // Per-window floating override
    bool fullscreen;
    int workspace;
    
    // Saved geometry for mode switching
    int saved_x, saved_y;
    int saved_width, saved_height;
};

struct keyboard {
    struct wl_list link;
    struct server *server;
    struct wlr_keyboard *wlr_keyboard;
    struct wl_listener key;
    struct wl_listener destroy;
    struct wl_listener modifiers;
};

/* core.c */
void server_new_output(struct wl_listener *listener, void *data);
void cursor_motion(struct wl_listener *listener, void *data);
void cursor_motion_abs(struct wl_listener *listener, void *data);
void cursor_axis(struct wl_listener *listener, void *data);
void cursor_frame(struct wl_listener *listener, void *data);
void request_cursor(struct wl_listener *listener, void *data);
void new_input(struct wl_listener *listener, void *data);
void server_new_xdg_popup(struct wl_listener *listener, void *data);
void server_new_xdg_decoration(struct wl_listener *listener, void *data);
void server_new_layer_surface(struct wl_listener *listener, void *data);

struct toplevel *toplevel_at(struct server *server, double lx, double ly,
                             struct wlr_surface **surface, double *sx, double *sy);

/* shell.c */
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
void cursor_button(struct wl_listener *listener, void *data);
bool handle_keybind(struct server *server, enum keybind_action action, const char *arg);

void arrange_windows(struct server *server);
void focus_toplevel(struct toplevel *toplevel);
struct toplevel *get_focused_toplevel(struct server *server);

#endif
