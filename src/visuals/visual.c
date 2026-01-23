/* decor_visual.c - Integration of visual system with window decorations */
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "../core.h"
#include "../config.h"
#include "../visual.h"
#include <stdlib.h>
#include <string.h>

/* Global visual configuration */
static visual_config_t *g_visual_config = NULL;
static render_surface_t *g_titlebar_surfaces[32] = {0};
static int g_surface_count = 0;

/* Initialize visual system with theme */
void visual_init(theme_preset_t preset) {
    if (g_visual_config) {
        free(g_visual_config);
    }
    
    g_visual_config = malloc(sizeof(visual_config_t));
    if (!g_visual_config) return;
    
    *g_visual_config = visual_config_preset(preset);
}

/* Load visual config from file */
int visual_load_config(const char *path) {
    if (!g_visual_config) {
        g_visual_config = malloc(sizeof(visual_config_t));
        if (!g_visual_config) return -1;
        *g_visual_config = visual_config_default();
    }
    
    return visual_config_load(g_visual_config, path);
}

/* Get current visual config */
visual_config_t *visual_get_config(void) {
    if (!g_visual_config) {
        g_visual_config = malloc(sizeof(visual_config_t));
        if (g_visual_config) {
            *g_visual_config = visual_config_default();
        }
    }
    return g_visual_config;
}

/* Apply theme preset */
void visual_set_theme(theme_preset_t preset) {
    if (!g_visual_config) {
        g_visual_config = malloc(sizeof(visual_config_t));
    }
    if (g_visual_config) {
        *g_visual_config = visual_config_preset(preset);
    }
}

/* Create decoration using visual system */
void decor_create_visual(struct toplevel *toplevel) {
    if (!config.decor.enabled) return;
    
    visual_config_t *vcfg = visual_get_config();
    if (!vcfg) return;
    
    struct server *server = toplevel->server;
    struct decoration *d = &toplevel->decor;
    
    /* Create decoration tree */
    d->tree = wlr_scene_tree_create(server->layer_windows);
    if (!d->tree) return;
    
    /* Create titlebar surface */
    int width = 800; /* Default width, will be updated */
    render_surface_t *rs = render_surface_create(d->tree, 
        width, vcfg->decoration.titlebar.height);
    
    if (!rs) {
        wlr_scene_node_destroy(&d->tree->node);
        d->tree = NULL;
        return;
    }
    
    /* Store surface for later updates */
    if (g_surface_count < 32) {
        g_titlebar_surfaces[g_surface_count++] = rs;
    }
    
    /* Store as void pointer to avoid type conflicts */
    d->rendered_titlebar = (struct rendered_titlebar *)rs;
    
    /* Reparent toplevel scene tree under decoration tree */
    wlr_scene_node_reparent(&toplevel->scene_tree->node, d->tree);
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, 
        vcfg->decoration.titlebar.height);
    
    d->width = width;
}

/* Update decoration using visual system */
void decor_update_visual(struct toplevel *toplevel, bool focused) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    visual_config_t *vcfg = visual_get_config();
    if (!vcfg) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    /* Get title */
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    /* Draw titlebar */
    draw_titlebar(rs, &vcfg->decoration.titlebar, title, focused, -1);
    render_surface_commit(rs);
}

/* Set titlebar size using visual system */
void decor_set_size_visual(struct toplevel *toplevel, int width) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    visual_config_t *vcfg = visual_get_config();
    if (!vcfg) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    toplevel->decor.width = width;
    
    /* Resize surface */
    render_surface_resize(rs, width, vcfg->decoration.titlebar.height);
    
    /* Redraw */
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    bool active = (toplevel == get_focused_toplevel(toplevel->server));
    draw_titlebar(rs, &vcfg->decoration.titlebar, title, active, -1);
    render_surface_commit(rs);
}

/* Destroy decoration using visual system */
void decor_destroy_visual(struct toplevel *toplevel) {
    if (!toplevel->decor.tree) return;
    
    render_surface_t *rs = (render_surface_t *)toplevel->decor.rendered_titlebar;
    if (rs) {
        /* Remove from global list */
        for (int i = 0; i < g_surface_count; i++) {
            if (g_titlebar_surfaces[i] == rs) {
                g_titlebar_surfaces[i] = g_titlebar_surfaces[--g_surface_count];
                break;
            }
        }
        render_surface_destroy(rs);
        toplevel->decor.rendered_titlebar = NULL;
    }
    
    wlr_scene_node_destroy(&toplevel->decor.tree->node);
    toplevel->decor.tree = NULL;
}

/* Hit test using visual system */
enum decor_hit decor_hit_test_visual(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return HIT_NONE;
    
    visual_config_t *vcfg = visual_get_config();
    if (!vcfg) return HIT_NONE;
    
    /* Get decoration position */
    int dx = toplevel->decor.tree->node.x;
    int dy = toplevel->decor.tree->node.y;
    
    /* Local coordinates */
    double x = lx - dx;
    double y = ly - dy;
    
    int h = vcfg->decoration.titlebar.height;
    int width = toplevel->decor.width;
    
    /* Check titlebar area */
    if (y >= 0 && y < h && x >= 0 && x < width) {
        titlebar_config_t *tb = &vcfg->decoration.titlebar;
        
        if (!tb->buttons_visible) return HIT_TITLEBAR;
        
        /* Calculate button positions */
        button_style_t *buttons[] = {
            &tb->btn_close,
            &tb->btn_maximize,
            &tb->btn_minimize,
        };
        
        int total_btn_width = 0;
        int visible_count = 0;
        
        for (int i = 0; i < 4 && tb->button_order[i] >= 0; i++) {
            int idx = tb->button_order[i];
            if (idx >= 0 && idx < 3) {
                total_btn_width += buttons[idx]->width;
                visible_count++;
            }
        }
        total_btn_width += tb->button_spacing * (visible_count - 1);
        
        int btn_x;
        if (tb->buttons_left) {
            btn_x = tb->button_margin;
        } else {
            btn_x = width - tb->button_margin - total_btn_width;
        }
        
        int btn_y = (h - buttons[0]->height) / 2;
        
        /* Check each button */
        for (int i = 0; i < 4 && tb->button_order[i] >= 0; i++) {
            int idx = tb->button_order[i];
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
            
            btn_x += btn->width + tb->button_spacing;
        }
        
        return HIT_TITLEBAR;
    }
    
    /* Check resize edges */
    int edge_size = 8;
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int total_height = h + geo.height;
    
    /* Corners */
    if (x < edge_size && y < edge_size) return HIT_RESIZE_TOP_LEFT;
    if (x >= width - edge_size && y < edge_size) return HIT_RESIZE_TOP_RIGHT;
    if (x < edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_LEFT;
    if (x >= width - edge_size && y >= total_height - edge_size) return HIT_RESIZE_BOTTOM_RIGHT;
    
    /* Edges */
    if (y < edge_size) return HIT_RESIZE_TOP;
    if (y >= total_height - edge_size) return HIT_RESIZE_BOTTOM;
    if (x < edge_size) return HIT_RESIZE_LEFT;
    if (x >= width - edge_size) return HIT_RESIZE_RIGHT;
    
    return HIT_NONE;
}

/* Update hover state for buttons */
void decor_update_hover_visual(struct toplevel *toplevel, double lx, double ly) {
    if (!config.decor.enabled || !toplevel->decor.tree) return;
    
    visual_config_t *vcfg = visual_get_config();
    if (!vcfg) return;
    
    render_surface_t *rs = toplevel->decor.rendered_titlebar;
    if (!rs) return;
    
    /* Get local coordinates */
    int dx = toplevel->decor.tree->node.x;
    int dy = toplevel->decor.tree->node.y;
    int x = (int)(lx - dx);
    int y = (int)(ly - dy);
    
    int h = vcfg->decoration.titlebar.height;
    
    /* Determine which button is hovered */
    int hover_button = -1;
    
    if (y >= 0 && y < h) {
        enum decor_hit hit = decor_hit_test_visual(toplevel, lx, ly);
        switch (hit) {
        case HIT_CLOSE: hover_button = 0; break;
        case HIT_MAXIMIZE: hover_button = 1; break;
        case HIT_MINIMIZE: hover_button = 2; break;
        default: break;
        }
    }
    
    /* Redraw with hover state */
    const char *title = toplevel->xdg_toplevel->title;
    if (!title) title = toplevel->xdg_toplevel->app_id;
    if (!title) title = "Untitled";
    
    bool active = (toplevel == get_focused_toplevel(toplevel->server));
    draw_titlebar(rs, &vcfg->decoration.titlebar, title, active, hover_button);
    render_surface_commit(rs);
}

/* Cleanup visual system */
void visual_cleanup(void) {
    for (int i = 0; i < g_surface_count; i++) {
        if (g_titlebar_surfaces[i]) {
            render_surface_destroy(g_titlebar_surfaces[i]);
            g_titlebar_surfaces[i] = NULL;
        }
    }
    g_surface_count = 0;
    
    if (g_visual_config) {
        free(g_visual_config);
        g_visual_config = NULL;
    }
}
