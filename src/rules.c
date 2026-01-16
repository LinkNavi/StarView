#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <string.h>
#include <fnmatch.h>

void apply_window_rules(struct toplevel *toplevel) {
    const char *app_id = toplevel->xdg_toplevel->app_id;
    const char *title = toplevel->xdg_toplevel->title;
    
    if (!app_id) app_id = "";
    if (!title) title = "";
    
    for (int i = 0; i < config.rule_count; i++) {
        struct window_rule *rule = &config.rules[i];
        
        // Match app_id (supports wildcards)
        if (rule->app_id[0] && fnmatch(rule->app_id, app_id, 0) != 0) {
            continue;
        }
        
        // Match title (supports wildcards)
        if (rule->title[0] && fnmatch(rule->title, title, 0) != 0) {
            continue;
        }
        
        // Apply rule
        if (rule->floating) {
            toplevel->floating = true;
        }
        
        if (rule->fullscreen) {
            toplevel->fullscreen = true;
        }
        
        if (rule->workspace > 0) {
            toplevel->workspace = rule->workspace;
        }
        
        if (rule->has_position) {
            struct wlr_scene_node *node = toplevel->decor.tree ? 
                &toplevel->decor.tree->node : &toplevel->scene_tree->node;
            wlr_scene_node_set_position(node, rule->x, rule->y);
        }
        
        if (rule->has_size) {
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 
                rule->width, rule->height);
        }
        
        if (rule->has_opacity) {
            toplevel->opacity = rule->opacity;
        }
    }
}
