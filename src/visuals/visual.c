/* decor_visual_simple.c - Simple integration using existing config */
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "../core.h"
#include "../config.h"
#include "../visual.h"
#include <stdlib.h>
#include <string.h>

/* Convert existing config to titlebar_config */
static titlebar_config_t config_to_titlebar(void) {
    titlebar_config_t tb = {0};
    
    /* Use existing config values */
    tb.height = config.decor.height;
    tb.padding_left = 10;
    tb.padding_right = 10;
    tb.corner_radius_tl = config.decor.corner_radius;
    tb.corner_radius_tr = config.decor.corner_radius;
    
    /* Colors from existing config */
    tb.bg_color = rgba_hex(config.decor.bg_color);
    tb.bg_color_inactive = rgba_hex(config.decor.bg_color_inactive);
    tb.bg_gradient.type = GRAD_NONE;
    
    /* Title style */
    tb.title_style = text_style_create(
        config.decor.font,
        config.decor.font_size,
        FONT_WEIGHT_NORMAL,
        rgba_hex(config.decor.title_color)
    );
    tb.title_style_inactive = text_style_create(
        config.decor.font,
        config.decor.font_size,
        FONT_WEIGHT_NORMAL,
        rgba_hex(config.decor.title_color_inactive)
    );
    tb.title_align = TEXT_ALIGN_CENTER;
    
    /* Buttons */
    tb.buttons_visible = true;
    tb.buttons_left = config.decor.buttons_left;
    tb.button_spacing = config.decor.button_spacing;
    tb.button_margin = config.decor.button_spacing;
    
    /* Create button styles from config colors */
    tb.btn_close = button_style_circle(
        config.decor.button_size,
        rgba_hex(config.decor.btn_close_color),
        ICON_CLOSE
    );
    tb.btn_close.states[BTN_STATE_HOVER].bg_color = rgba_hex(config.decor.btn_close_hover);
    
    tb.btn_maximize = button_style_circle(
        config.decor.button_size,
        rgba_hex(config.decor.btn_max_color),
        ICON_MAXIMIZE
    );
    tb.btn_maximize.states[BTN_STATE_HOVER].bg_color = rgba_hex(config.decor.btn_max_hover);
    
    tb.btn_minimize = button_style_circle(
        config.decor.button_size,
        rgba_hex(config.decor.btn_min_color),
        ICON_MINIMIZE
    );
    tb.btn_minimize.states[BTN_STATE_HOVER].bg_color = rgba_hex(config.decor.btn_min_hover);
    
    /* Button order */
    if (tb.buttons_left) {
        tb.button_order[0] = 0; // close
        tb.button_order[1] = 2; // minimize
        tb.button_order[2] = 1; // maximize
    } else {
        tb.button_order[0] = 2; // minimize
        tb.button_order[1] = 1; // maximize
        tb.button_order[2] = 0; // close
    }
    tb.button_order[3] = -1;
    
    tb.border = border_none();
    tb.shadow = shadow_none();
    tb.separator_visible = false;
    tb.transition_ms = 150;
    
    return tb;
}

void decor_create_visual(struct toplevel *toplevel) {
    if (!config.decor.enabled) return;
    
    struct server *server = toplevel->server;
    struct decoration *d = &toplevel->decor;
    
    /* Create decoration tree */
    d->tree = wlr_scene_tree_create(server->layer_windows);
    if (!d->tree) return;
    
    /* Create titlebar surface */
    titlebar_config_t tb_config = config_to_titlebar();
    int width = 800;
    
    render_surface_t *rs = render_surface_create(d->tree, width, tb_config.height);
    if (!rs) {
        wlr_scene_node_destroy(&d->tree->node);
        d->tree = NULL;
        return;
    }
    
    /* Store as rendered_titlebar (type cast to void* compatible struct) */
    d->rendered_titlebar = (struct rendered_titlebar *)rs;
    
    /* Reparent toplevel scene tree */
    wlr_scene_node_reparent(&toplevel->scene_tree->node, d->tree);
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, tb_config.height);
    
    d->width = width;
}

void decor_update_visual(struct toplevel *toplevel, bool focused) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    titlebar_config_t tb_config = config_to_titlebar();
    
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    draw_titlebar(rs, &tb_config, title, focused, -1);
    render_surface_commit(rs);
}

void decor_set_size_visual(struct toplevel *toplevel, int width) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    titlebar_config_t tb_config = config_to_titlebar();
    toplevel->decor.width = width;
    
    render_surface_resize(rs, width, tb_config.height);
    
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    bool active = (toplevel == get_focused_toplevel(toplevel->server));
    draw_titlebar(rs, &tb_config, title, active, -1);
    render_surface_commit(rs);
}

void decor_destroy_visual(struct toplevel *toplevel) {
    if (!toplevel->decor.tree) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (rs) {
        render_surface_destroy(rs);
        toplevel->decor.rendered_titlebar = NULL;
    }
    
    wlr_scene_node_destroy(&toplevel->decor.tree->node);
    toplevel->decor.tree = NULL;
}

enum decor_hit decor_hit_test_visual(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return HIT_NONE;
    
    titlebar_config_t tb_config = config_to_titlebar();
    
    int dx = toplevel->decor.tree->node.x;
    int dy = toplevel->decor.tree->node.y;
    double x = lx - dx;
    double y = ly - dy;
    
    int h = tb_config.height;
    int width = toplevel->decor.width;
    
    /* Titlebar area */
    if (y >= 0 && y < h && x >= 0 && x < width) {
        if (!tb_config.buttons_visible) return HIT_TITLEBAR;
        
        /* Calculate button positions */
        button_style_t *buttons[] = {
            &tb_config.btn_close,
            &tb_config.btn_maximize,
            &tb_config.btn_minimize,
        };
        
        int total_btn_width = 0;
        for (int i = 0; i < 3; i++) {
            total_btn_width += buttons[i]->width;
        }
        total_btn_width += tb_config.button_spacing * 2;
        
        int btn_x = tb_config.buttons_left ? 
                    tb_config.button_margin : 
                    width - tb_config.button_margin - total_btn_width;
        int btn_y = (h - buttons[0]->height) / 2;
        
        for (int i = 0; i < 3; i++) {
            int idx = tb_config.button_order[i];
            if (idx < 0 || idx >= 3) continue;
            
            button_style_t *btn = buttons[idx];
            
            if (x >= btn_x && x < btn_x + btn->width &&
                y >= btn_y && y < btn_y + btn->height) {
                switch (idx) {
                case 0: return HIT_CLOSE;
                case 1: return HIT_MAXIMIZE;
                case 2: return HIT_MINIMIZE;
                }
            }
            
            btn_x += btn->width + tb_config.button_spacing;
        }
        
        return HIT_TITLEBAR;
    }
    
    /* Resize edges */
    int edge_size = 8;
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int total_height = h + geo.height;
    
    if (x < edge_size && y < edge_size) return HIT_RESIZE_TOP_LEFT;
    if (x >= width - edge_size && y < edge_size) return HIT_RESIZE_TOP_RIGHT;
    if (x < edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_LEFT;
    if (x >= width - edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_RIGHT;
    
    if (y < edge_size) return HIT_RESIZE_TOP;
    if (y >= total_height - edge_size) return HIT_RESIZE_BOTTOM;
    if (x < edge_size) return HIT_RESIZE_LEFT;
    if (x >= width - edge_size) return HIT_RESIZE_RIGHT;
    
    return HIT_NONE;
}

void decor_update_hover_visual(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    titlebar_config_t tb_config = config_to_titlebar();
    
    int dx = toplevel->decor.tree->node.x;
    int dy = toplevel->decor.tree->node.y;
    int y = (int)(ly - dy);
    
    int hover_button = -1;
    
    if (y >= 0 && y < tb_config.height) {
        enum decor_hit hit = decor_hit_test_visual(toplevel, lx, ly);
        switch (hit) {
        case HIT_CLOSE: hover_button = 0; break;
        case HIT_MAXIMIZE: hover_button = 1; break;
        case HIT_MINIMIZE: hover_button = 2; break;
        default: break;
        }
    }
    
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    bool active = (toplevel == get_focused_toplevel(toplevel->server));
    draw_titlebar(rs, &tb_config, title, active, hover_button);
    render_surface_commit(rs);
}
