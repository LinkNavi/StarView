/* decor.c - Window decoration using configurable visual system */
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "../core.h"
#include "../config.h"
#include "../titlebar_render.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

/* Global theme instance - can be changed at runtime */
static struct titlebar_theme *g_theme = NULL;
static enum theme_preset g_current_preset = THEME_PRESET_DEFAULT;

/* Ensure theme is initialized */
static void ensure_theme_initialized(void) {
    if (g_theme) return;
    
    g_theme = titlebar_theme_create();
    if (g_theme) {
        titlebar_theme_load_preset(g_theme, g_current_preset);
        titlebar_set_global_theme(g_theme);
    }
}

/* Set theme preset - can be called from config or keybindings */
void decor_set_theme_preset(enum theme_preset preset) {
    g_current_preset = preset;
    
    if (g_theme) {
        titlebar_theme_load_preset(g_theme, preset);
    } else {
        ensure_theme_initialized();
    }
}

/* Get current theme for external modification */
struct titlebar_theme *decor_get_theme(void) {
    ensure_theme_initialized();
    return g_theme;
}

/* Color conversion helper */
static void color_to_float(uint32_t color, float out[4]) {
    out[0] = ((color >> 24) & 0xff) / 255.0f;
    out[1] = ((color >> 16) & 0xff) / 255.0f;
    out[2] = ((color >> 8) & 0xff) / 255.0f;
    out[3] = (color & 0xff) / 255.0f;
}

void decor_create(struct toplevel *toplevel) {
    if (!config.decor.enabled) return;
    
    ensure_theme_initialized();
    
    struct server *server = toplevel->server;
    struct decoration *d = &toplevel->decor;
    
    /* Create decoration tree as parent of window */
    d->tree = wlr_scene_tree_create(server->layer_windows);
    if (!d->tree) return;
    
    int h = g_theme ? g_theme->height : config.decor.height;
    int bw = config.border_width;
    
    /* Create Cairo-rendered titlebar */
    if (g_theme) {
        d->rendered_titlebar = titlebar_render_create(d->tree, g_theme);
    }
    
    /* Create borders using scene rects */
    float border_color[4];
    color_to_float(config.border_color_active, border_color);
    
    d->border_top = wlr_scene_rect_create(d->tree, 100, bw, border_color);
    d->border_bottom = wlr_scene_rect_create(d->tree, 100, bw, border_color);
    d->border_left = wlr_scene_rect_create(d->tree, bw, 100, border_color);
    d->border_right = wlr_scene_rect_create(d->tree, bw, 100, border_color);
    
    /* Reparent toplevel scene tree under decoration tree */
    wlr_scene_node_reparent(&toplevel->scene_tree->node, d->tree);
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, h);
    
    d->width = 100;
}

void decor_set_size(struct toplevel *toplevel, int width) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    struct decoration *d = &toplevel->decor;
    
    int h = g_theme ? g_theme->height : config.decor.height;
    int bw = config.border_width;
    
    d->width = width;
    
    /* Update Cairo titlebar */
    if (d->rendered_titlebar && g_theme) {
        const char *title = toplevel->xdg_toplevel->title;
        if (!title) title = toplevel->xdg_toplevel->app_id;
        if (!title) title = "Untitled";
        
        bool active = (toplevel == get_focused_toplevel(toplevel->server));
        titlebar_render_update(d->rendered_titlebar, g_theme, width, title, active);
    }
    
    /* Get actual content height */
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int content_height = geo.height > 0 ? geo.height : 100;
    int total_height = h + content_height;
    
    /* Resize borders */
    wlr_scene_rect_set_size(d->border_top, width + bw * 2, bw);
    wlr_scene_node_set_position(&d->border_top->node, -bw, -bw);
    
    wlr_scene_rect_set_size(d->border_bottom, width + bw * 2, bw);
    wlr_scene_node_set_position(&d->border_bottom->node, -bw, total_height);
    
    wlr_scene_rect_set_size(d->border_left, bw, total_height + bw * 2);
    wlr_scene_node_set_position(&d->border_left->node, -bw, -bw);
    
    wlr_scene_rect_set_size(d->border_right, bw, total_height + bw * 2);
    wlr_scene_node_set_position(&d->border_right->node, width, -bw);
}

void decor_update(struct toplevel *toplevel, bool focused) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    struct decoration *d = &toplevel->decor;
    
    /* Update Cairo titlebar for focus state */
    if (d->rendered_titlebar && g_theme) {
        const char *title = toplevel->xdg_toplevel->title;
        if (!title) title = toplevel->xdg_toplevel->app_id;
        if (!title) title = "Untitled";
        
        titlebar_render_update(d->rendered_titlebar, g_theme, d->width, title, focused);
    }
    
    /* Update border colors */
    float border[4];
    if (focused) {
        color_to_float(config.border_color_active, border);
    } else {
        color_to_float(config.border_color_inactive, border);
    }
    
    wlr_scene_rect_set_color(d->border_top, border);
    wlr_scene_rect_set_color(d->border_bottom, border);
    wlr_scene_rect_set_color(d->border_left, border);
    wlr_scene_rect_set_color(d->border_right, border);
}

void decor_destroy(struct toplevel *toplevel) {
    if (!toplevel->decor.tree) return;
    
    if (toplevel->decor.rendered_titlebar) {
        titlebar_render_destroy(toplevel->decor.rendered_titlebar);
        toplevel->decor.rendered_titlebar = NULL;
    }
    
    wlr_scene_node_destroy(&toplevel->decor.tree->node);
    toplevel->decor.tree = NULL;
}

enum decor_hit decor_hit_test(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return HIT_NONE;
    
    struct decoration *d = &toplevel->decor;
    
    /* Get decoration position */
    int dx = d->tree->node.x;
    int dy = d->tree->node.y;
    
    /* Local coordinates */
    double x = lx - dx;
    double y = ly - dy;
    
    int h = g_theme ? g_theme->height : config.decor.height;
    int bw = config.border_width;
    int width = d->width;
    
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int total_height = h + geo.height;
    
    /* Check resize edges first */
    int edge_size = 8;
    
    /* Corners */
    if (x < edge_size && y < edge_size) return HIT_RESIZE_TOP_LEFT;
    if (x >= width - edge_size && y < edge_size) return HIT_RESIZE_TOP_RIGHT;
    if (x < edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_LEFT;
    if (x >= width - edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_RIGHT;
    
    /* Edges */
    if (y < bw) return HIT_RESIZE_TOP;
    if (y >= total_height - bw) return HIT_RESIZE_BOTTOM;
    if (x < bw) return HIT_RESIZE_LEFT;
    if (x >= width - bw) return HIT_RESIZE_RIGHT;
    
    /* Titlebar area - use Cairo titlebar hit testing */
    if (y >= 0 && y < h && d->rendered_titlebar) {
        enum button_type btn = titlebar_render_hit_test(d->rendered_titlebar, (int)x, (int)y);
        
        switch (btn) {
        case BTN_TYPE_CLOSE:
            return HIT_CLOSE;
        case BTN_TYPE_MAXIMIZE:
            return HIT_MAXIMIZE;
        case BTN_TYPE_MINIMIZE:
            return HIT_MINIMIZE;
        default:
            return HIT_TITLEBAR;
        }
    }
    
    return HIT_NONE;
}

void decor_update_hover(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    struct decoration *d = &toplevel->decor;
    
    if (!d->rendered_titlebar) return;
    
    /* Get local coordinates */
    int dx = d->tree->node.x;
    int dy = d->tree->node.y;
    int x = (int)(lx - dx);
    int y = (int)(ly - dy);
    
    int h = g_theme ? g_theme->height : config.decor.height;
    
    /* Reset all to normal first */
    enum button_state close_state = BTN_STATE_NORMAL;
    enum button_state max_state = BTN_STATE_NORMAL;
    enum button_state min_state = BTN_STATE_NORMAL;
    
    /* Check if in titlebar */
    if (y >= 0 && y < h) {
        enum button_type btn = titlebar_render_hit_test(d->rendered_titlebar, x, y);
        
        switch (btn) {
        case BTN_TYPE_CLOSE:
            close_state = BTN_STATE_HOVER;
            break;
        case BTN_TYPE_MAXIMIZE:
            max_state = BTN_STATE_HOVER;
            break;
        case BTN_TYPE_MINIMIZE:
            min_state = BTN_STATE_HOVER;
            break;
        default:
            break;
        }
    }
    
    /* Update button states */
    bool changed = false;
    if (d->hovered_close != (close_state == BTN_STATE_HOVER)) {
        d->hovered_close = (close_state == BTN_STATE_HOVER);
        titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_CLOSE, close_state);
        changed = true;
    }
    if (d->hovered_max != (max_state == BTN_STATE_HOVER)) {
        d->hovered_max = (max_state == BTN_STATE_HOVER);
        titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_MAXIMIZE, max_state);
        changed = true;
    }
    if (d->hovered_min != (min_state == BTN_STATE_HOVER)) {
        d->hovered_min = (min_state == BTN_STATE_HOVER);
        titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_MINIMIZE, min_state);
        changed = true;
    }
    
    (void)changed;
}
