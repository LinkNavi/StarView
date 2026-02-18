#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "core.h"
#include "background.h"
#include "titlebar_render.h"
#include "config_live.h"
#include <stdio.h>

void config_apply_live(struct server *server) {
    if (!server) return;

    printf("[LIVE] Applying config changes...\n");

    /* 1. Update titlebar theme */
    struct titlebar_theme *theme = titlebar_get_global_theme();
    if (theme) {
        titlebar_theme_load_from_config(theme, &config.decor);
        printf("[LIVE] Titlebar theme updated\n");
    }

    /* 2. Update all decorations */
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        bool focused = (toplevel == get_focused_toplevel(server));
        if (config.decor.enabled && !toplevel->decor.tree)
            decor_create(toplevel);
        else if (!config.decor.enabled && toplevel->decor.tree)
            decor_destroy(toplevel);
        if (toplevel->decor.tree) {
            struct wlr_box geo;
            wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
            decor_set_size(toplevel, geo.width);
            decor_update(toplevel, focused);
        }
    }
    printf("[LIVE] Decorations updated\n");

    /* 3. Update backgrounds */
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        if (config.background.enabled) {
            int w = output->wlr_output->width;
            int h = output->wlr_output->height;
            if (output->background)
                background_update(output->background, w, h, &config.background);
            else
                output->background = background_create(server->layer_bg, w, h, &config.background);
        } else if (output->background) {
            wlr_scene_node_set_enabled(&output->background->node, false);
        }
    }
    printf("[LIVE] Backgrounds updated\n");

    /* 4. Mode + layout */
    server->mode = config.default_mode;
    arrange_windows(server);
    printf("[LIVE] Done\n");
}
