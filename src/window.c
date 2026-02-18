#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "window.h"
#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/xwayland.h>

struct wlr_surface *window_get_surface(struct window *win) {
    if (!win) return NULL;
    switch (win->type) {
    case WIN_XDG:
        if (win->xdg.xdg_toplevel && win->xdg.xdg_toplevel->base)
            return win->xdg.xdg_toplevel->base->surface;
        return NULL;
    case WIN_XWAYLAND:
        if (win->xwl.xwayland_surface)
            return win->xwl.xwayland_surface->surface;
        return NULL;
    }
    return NULL;
}

const char *window_get_title(struct window *win) {
    if (!win) return "Untitled";
    switch (win->type) {
    case WIN_XDG:
        if (win->xdg.xdg_toplevel) {
            const char *t = win->xdg.xdg_toplevel->title;
            if (t) return t;
            t = win->xdg.xdg_toplevel->app_id;
            if (t) return t;
        }
        return "Untitled";
    case WIN_XWAYLAND:
        if (win->xwl.xwayland_surface) {
            const char *t = win->xwl.xwayland_surface->title;
            if (t) return t;
            t = win->xwl.xwayland_surface->class;
            if (t) return t;
        }
        return "X11 Window";
    }
    return "Untitled";
}

const char *window_get_app_id(struct window *win) {
    if (!win) return "";
    switch (win->type) {
    case WIN_XDG:
        if (win->xdg.xdg_toplevel && win->xdg.xdg_toplevel->app_id)
            return win->xdg.xdg_toplevel->app_id;
        return "";
    case WIN_XWAYLAND:
        if (win->xwl.xwayland_surface && win->xwl.xwayland_surface->class)
            return win->xwl.xwayland_surface->class;
        return "";
    }
    return "";
}

struct wlr_scene_node *window_get_scene_node(struct window *win) {
    if (!win) return NULL;
    if (win->decor.tree)
        return &win->decor.tree->node;
    if (win->scene_tree)
        return &win->scene_tree->node;
    return NULL;
}

void window_get_geometry(struct window *win, int *x, int *y, int *w, int *h) {
    if (!win) { *x=*y=*w=*h=0; return; }

    struct wlr_scene_node *node = window_get_scene_node(win);
    if (x) *x = node ? node->x : 0;
    if (y) *y = node ? node->y : 0;

    switch (win->type) {
    case WIN_XDG: {
        struct wlr_box geo;
        wlr_xdg_surface_get_geometry(win->xdg.xdg_toplevel->base, &geo);
        if (w) *w = geo.width > 0 ? geo.width : 100;
        if (h) *h = geo.height > 0 ? geo.height : 100;
        break;
    }
    case WIN_XWAYLAND:
        if (w) *w = win->xwl.xwayland_surface->width > 0 ? win->xwl.xwayland_surface->width : 100;
        if (h) *h = win->xwl.xwayland_surface->height > 0 ? win->xwl.xwayland_surface->height : 100;
        break;
    }
}

void window_set_size(struct window *win, int w, int h) {
    if (!win || w <= 0 || h <= 0) return;
    switch (win->type) {
    case WIN_XDG:
        wlr_xdg_toplevel_set_size(win->xdg.xdg_toplevel, w, h);
        break;
    case WIN_XWAYLAND:
        wlr_xwayland_surface_configure(win->xwl.xwayland_surface,
            win->xwl.xwayland_surface->x, win->xwl.xwayland_surface->y, w, h);
        break;
    }
}

void window_set_position(struct window *win, int x, int y) {
    if (!win) return;
    struct wlr_scene_node *node = window_get_scene_node(win);
    if (node) wlr_scene_node_set_position(node, x, y);

    /* For X11 we also need to update the X surface position */
    if (win->type == WIN_XWAYLAND && win->xwl.xwayland_surface) {
        int w, h;
        window_get_geometry(win, NULL, NULL, &w, &h);
        wlr_xwayland_surface_configure(win->xwl.xwayland_surface, x, y, w, h);
    }
}

void window_close(struct window *win) {
    if (!win) return;
    switch (win->type) {
    case WIN_XDG:
        wlr_xdg_toplevel_send_close(win->xdg.xdg_toplevel);
        break;
    case WIN_XWAYLAND:
        if (win->xwl.xwayland_surface)
            wlr_xwayland_surface_close(win->xwl.xwayland_surface);
        break;
    }
}

void window_set_fullscreen(struct window *win, bool fs) {
    if (!win) return;
    win->fullscreen = fs;
    switch (win->type) {
    case WIN_XDG:
        wlr_xdg_toplevel_set_fullscreen(win->xdg.xdg_toplevel, fs);
        break;
    case WIN_XWAYLAND:
        /* XWayland handles fullscreen via configure */
        break;
    }
}

void window_set_activated(struct window *win, bool active) {
    if (!win) return;
    switch (win->type) {
    case WIN_XDG:
        wlr_xdg_toplevel_set_activated(win->xdg.xdg_toplevel, active);
        break;
    case WIN_XWAYLAND:
        if (win->xwl.xwayland_surface)
            wlr_xwayland_surface_activate(win->xwl.xwayland_surface, active);
        break;
    }
}

void window_set_resizing(struct window *win, bool resizing) {
    if (!win) return;
    if (win->type == WIN_XDG) {
        wlr_xdg_toplevel_set_resizing(win->xdg.xdg_toplevel, resizing);
    }
}

void window_raise(struct window *win) {
    if (!win) return;
    struct wlr_scene_node *node = window_get_scene_node(win);
    if (node) wlr_scene_node_raise_to_top(node);
}

bool window_is_visible(struct window *win, int workspace) {
    if (!win || win->minimized) return false;
    return win->workspace == workspace;
}

bool window_is_unmanaged(struct window *win) {
    if (!win) return false;
    return (win->type == WIN_XWAYLAND && win->xwl.override_redirect);
}
