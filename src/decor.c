#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include "titlebar_render.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

/* Global theme instance */
static struct titlebar_theme *g_theme = NULL;

static void ensure_theme_initialized(void) {
    if (g_theme) return;
    
    g_theme = titlebar_theme_create();
    if (g_theme) {
        /* Load theme based on config - you can change this */
        titlebar_theme_load_preset(g_theme, THEME_PRESET_DEFAULT);
        titlebar_set_global_theme(g_theme);
    }
}
#include "decor_visual.h"

// Just redirect to the visual system
void decor_create(struct toplevel *toplevel) {
    decor_create_visual(toplevel);
}

void decor_update(struct toplevel *toplevel, bool focused) {
    decor_update_visual(toplevel, focused);
}

void decor_set_size(struct toplevel *toplevel, int width) {
    decor_set_size_visual(toplevel, width);
}

void decor_destroy(struct toplevel *toplevel) {
    decor_destroy_visual(toplevel);
}

enum decor_hit decor_hit_test(struct toplevel *toplevel, double x, double y) {
    return decor_hit_test_visual(toplevel, x, y);
}

void decor_update_hover(struct toplevel *toplevel, double x, double y) {
    decor_update_hover_visual(toplevel, x, y);
}
static void color_to_float(uint32_t color, float out[4]) {
    out[0] = ((color >> 24) & 0xff) / 255.0f;
    out[1] = ((color >> 16) & 0xff) / 255.0f;
    out[2] = ((color >> 8) & 0xff) / 255.0f;
    out[3] = (color & 0xff) / 255.0f;
}
/* Public function to change theme preset at runtime */
void decor_set_theme_preset(enum theme_preset preset) {
    ensure_theme_initialized();
    if (g_theme) {
        titlebar_theme_load_preset(g_theme, preset);
    }
}

/* Get current theme for external modification */
struct titlebar_theme *decor_get_theme(void) {
    ensure_theme_initialized();
    return g_theme;
}
