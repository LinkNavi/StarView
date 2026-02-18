#ifndef WINDOW_H
#define WINDOW_H

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

/* Forward declarations */
struct server;
struct wlr_xwayland_surface;
struct decoration;
struct animation;

/*
 * Unified window type - wraps both XDG and XWayland surfaces
 */
enum window_type {
    WIN_XDG,
    WIN_XWAYLAND,
};

struct window {
    struct wl_list link;         /* server->windows */
    struct server *server;
    enum window_type type;

    /* Scene graph */
    struct wlr_scene_tree *scene_tree;

    /* Common state */
    bool floating;
    bool fullscreen;
    bool minimized;
    bool maximized;
    int workspace;
    float opacity;

    /* Saved geometry for restore */
    int saved_x, saved_y;
    int saved_width, saved_height;
    int pre_max_x, pre_max_y;
    int pre_max_width, pre_max_height;

    /* Decoration & animation - using pointers to avoid circular dependency */
    struct decoration *decor;
    struct animation *anim;

    /* Type-specific data */
    union {
        struct {
            struct wlr_xdg_toplevel *xdg_toplevel;
            struct wl_listener map;
            struct wl_listener unmap;
            struct wl_listener commit;
            struct wl_listener destroy;
            struct wl_listener request_fullscreen;
            struct wl_listener request_minimize;
        } xdg;

        struct {
            struct wlr_xwayland_surface *xwayland_surface;
            bool override_redirect;
            bool is_popup;
            bool is_dialog;
            bool is_splash;
            struct window *parent;

            struct wl_listener associate;
            struct wl_listener dissociate;
            struct wl_listener surface_map;
            struct wl_listener unmap;
            struct wl_listener destroy;
            struct wl_listener request_configure;
            struct wl_listener request_fullscreen;
            struct wl_listener request_minimize;
            struct wl_listener request_activate;
            struct wl_listener request_move;
            struct wl_listener request_resize;
            struct wl_listener set_title;
            struct wl_listener set_class;
            struct wl_listener set_parent;
            struct wl_listener set_hints;
            struct wl_listener set_window_type;
            struct wl_listener set_override_redirect;
        } xwl;
    };
};

/* ---- Unified window API ---- */

/* Get the wlr_surface for keyboard focus */
struct wlr_surface *window_get_surface(struct window *win);

/* Get title string */
const char *window_get_title(struct window *win);

/* Get app_id / class */
const char *window_get_app_id(struct window *win);

/* Get the scene node to position (accounts for decorations) */
struct wlr_scene_node *window_get_scene_node(struct window *win);

/* Get content geometry (width/height of the client area) */
void window_get_geometry(struct window *win, int *x, int *y, int *w, int *h);

/* Set size request */
void window_set_size(struct window *win, int w, int h);

/* Set position (moves decor tree or scene tree) */
void window_set_position(struct window *win, int x, int y);

/* Send close */
void window_close(struct window *win);

/* Set fullscreen */
void window_set_fullscreen(struct window *win, bool fs);

/* Set activated / focused */
void window_set_activated(struct window *win, bool active);

/* Set resizing hint */
void window_set_resizing(struct window *win, bool resizing);

/* Raise to top */
void window_raise(struct window *win);

/* Check if window is on given workspace and visible */
bool window_is_visible(struct window *win, int workspace);

/* Check if this is an override-redirect X11 window */
bool window_is_unmanaged(struct window *win);

#endif /* WINDOW_H */
