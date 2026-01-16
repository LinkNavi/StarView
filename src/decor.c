#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <wlr/types/wlr_scene.h>

static void color_to_float(uint32_t color, float out[4]) {
    out[0] = ((color >> 24) & 0xff) / 255.0f;
    out[1] = ((color >> 16) & 0xff) / 255.0f;
    out[2] = ((color >> 8) & 0xff) / 255.0f;
    out[3] = (color & 0xff) / 255.0f;
}

void decor_create(struct toplevel *toplevel) {
    if (!config.decor.enabled) return;
    
    struct server *server = toplevel->server;
    struct decoration *d = &toplevel->decor;
    
    // Create decoration tree as parent of window
    d->tree = wlr_scene_tree_create(server->layer_windows);
    if (!d->tree) return;
    
    float bg[4], close[4], max[4], min[4];
    color_to_float(config.decor.bg_color, bg);
    color_to_float(config.decor.btn_close_color, close);
    color_to_float(config.decor.btn_max_color, max);
    color_to_float(config.decor.btn_min_color, min);
    
    int h = config.decor.height;
    int btn_size = config.decor.button_size;
    int btn_spacing = config.decor.button_spacing;
    int bw = config.border_width;
    
    // Titlebar
    d->titlebar = wlr_scene_rect_create(d->tree, 100, h, bg);
    wlr_scene_node_set_position(&d->titlebar->node, 0, 0);
    
    // Buttons
    if (config.decor.buttons_left) {
        // macOS style: close, minimize, maximize on left
        d->btn_close = wlr_scene_rect_create(d->tree, btn_size, btn_size, close);
        wlr_scene_node_set_position(&d->btn_close->node, btn_spacing, (h - btn_size) / 2);
        
        d->btn_min = wlr_scene_rect_create(d->tree, btn_size, btn_size, min);
        wlr_scene_node_set_position(&d->btn_min->node, btn_spacing * 2 + btn_size, (h - btn_size) / 2);
        
        d->btn_max = wlr_scene_rect_create(d->tree, btn_size, btn_size, max);
        wlr_scene_node_set_position(&d->btn_max->node, btn_spacing * 3 + btn_size * 2, (h - btn_size) / 2);
    } else {
        // Windows style: minimize, maximize, close on right (positioned later in set_size)
        d->btn_min = wlr_scene_rect_create(d->tree, btn_size, btn_size, min);
        d->btn_max = wlr_scene_rect_create(d->tree, btn_size, btn_size, max);
        d->btn_close = wlr_scene_rect_create(d->tree, btn_size, btn_size, close);
    }
    
    // Borders
    float border_color[4];
    color_to_float(config.border_color_active, border_color);
    
    d->border_top = wlr_scene_rect_create(d->tree, 100, bw, border_color);
    d->border_bottom = wlr_scene_rect_create(d->tree, 100, bw, border_color);
    d->border_left = wlr_scene_rect_create(d->tree, bw, 100, border_color);
    d->border_right = wlr_scene_rect_create(d->tree, bw, 100, border_color);
    
    // Reparent toplevel scene tree under decoration tree
    wlr_scene_node_reparent(&toplevel->scene_tree->node, d->tree);
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, h);
    
    d->width = 100;
}

void decor_set_size(struct toplevel *toplevel, int width) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    struct decoration *d = &toplevel->decor;
    
    int h = config.decor.height;
    int btn_size = config.decor.button_size;
    int btn_spacing = config.decor.button_spacing;
    int bw = config.border_width;
    
    d->width = width;
    
    // Resize titlebar
    wlr_scene_rect_set_size(d->titlebar, width, h);
    
    // Position buttons on right side (Windows style)
    if (!config.decor.buttons_left) {
        int x = width - btn_spacing - btn_size;
        wlr_scene_node_set_position(&d->btn_close->node, x, (h - btn_size) / 2);
        x -= btn_spacing + btn_size;
        wlr_scene_node_set_position(&d->btn_max->node, x, (h - btn_size) / 2);
        x -= btn_spacing + btn_size;
        wlr_scene_node_set_position(&d->btn_min->node, x, (h - btn_size) / 2);
    }
    
    // Get actual content height
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int content_height = geo.height > 0 ? geo.height : 100;
    int total_height = h + content_height;
    
    // Resize borders
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
    
    float bg[4], border[4];
    if (focused) {
        color_to_float(config.decor.bg_color, bg);
        color_to_float(config.border_color_active, border);
    } else {
        color_to_float(config.decor.bg_color_inactive, bg);
        color_to_float(config.border_color_inactive, border);
    }
    
    wlr_scene_rect_set_color(d->titlebar, bg);
    wlr_scene_rect_set_color(d->border_top, border);
    wlr_scene_rect_set_color(d->border_bottom, border);
    wlr_scene_rect_set_color(d->border_left, border);
    wlr_scene_rect_set_color(d->border_right, border);
}

void decor_destroy(struct toplevel *toplevel) {
    if (!toplevel->decor.tree) return;
    wlr_scene_node_destroy(&toplevel->decor.tree->node);
    toplevel->decor.tree = NULL;
}

enum decor_hit decor_hit_test(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return HIT_NONE;
    
    struct decoration *d = &toplevel->decor;
    
    // Get decoration position
    int dx = d->tree->node.x;
    int dy = d->tree->node.y;
    
    // Local coordinates
    double x = lx - dx;
    double y = ly - dy;
    
    int h = config.decor.height;
    int btn_size = config.decor.button_size;
    int btn_spacing = config.decor.button_spacing;
    int bw = config.border_width;
    int width = d->width;
    
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int total_height = h + geo.height;
    
    // Check resize edges first
    int edge_size = 8;
    
    // Corners
    if (x < edge_size && y < edge_size) return HIT_RESIZE_TOP_LEFT;
    if (x >= width - edge_size && y < edge_size) return HIT_RESIZE_TOP_RIGHT;
    if (x < edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_LEFT;
    if (x >= width - edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_RIGHT;
    
    // Edges
    if (y < bw) return HIT_RESIZE_TOP;
    if (y >= total_height - bw) return HIT_RESIZE_BOTTOM;
    if (x < bw) return HIT_RESIZE_LEFT;
    if (x >= width - bw) return HIT_RESIZE_RIGHT;
    
    // Titlebar area
    if (y >= 0 && y < h) {
        // Check buttons
        if (config.decor.buttons_left) {
            // macOS style
            if (x >= btn_spacing && x < btn_spacing + btn_size &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_CLOSE;
            }
            if (x >= btn_spacing * 2 + btn_size && x < btn_spacing * 2 + btn_size * 2 &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_MINIMIZE;
            }
            if (x >= btn_spacing * 3 + btn_size * 2 && x < btn_spacing * 3 + btn_size * 3 &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_MAXIMIZE;
            }
        } else {
            // Windows style
            int bx = width - btn_spacing - btn_size;
            if (x >= bx && x < bx + btn_size &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_CLOSE;
            }
            bx -= btn_spacing + btn_size;
            if (x >= bx && x < bx + btn_size &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_MAXIMIZE;
            }
            bx -= btn_spacing + btn_size;
            if (x >= bx && x < bx + btn_size &&
                y >= (h - btn_size) / 2 && y < (h + btn_size) / 2) {
                return HIT_MINIMIZE;
            }
        }
        
        return HIT_TITLEBAR;
    }
    
    return HIT_NONE;
}

void decor_update_hover(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    struct decoration *d = &toplevel->decor;
    enum decor_hit hit = decor_hit_test(toplevel, lx, ly);
    
    float close_color[4], max_color[4], min_color[4];
    
    // Update close button
    bool close_hover = (hit == HIT_CLOSE);
    if (close_hover != d->hovered_close) {
        d->hovered_close = close_hover;
        color_to_float(close_hover ? config.decor.btn_close_hover : config.decor.btn_close_color, close_color);
        wlr_scene_rect_set_color(d->btn_close, close_color);
    }
    
    // Update maximize button
    bool max_hover = (hit == HIT_MAXIMIZE);
    if (max_hover != d->hovered_max) {
        d->hovered_max = max_hover;
        color_to_float(max_hover ? config.decor.btn_max_hover : config.decor.btn_max_color, max_color);
        wlr_scene_rect_set_color(d->btn_max, max_color);
    }
    
    // Update minimize button
    bool min_hover = (hit == HIT_MINIMIZE);
    if (min_hover != d->hovered_min) {
        d->hovered_min = min_hover;
        color_to_float(min_hover ? config.decor.btn_min_hover : config.decor.btn_min_color, min_color);
        wlr_scene_rect_set_color(d->btn_min, min_color);
    }
}
