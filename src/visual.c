/* Stub render surface implementation */
#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "visual_types.h"

/* Stub render surface type */
struct render_surface_t {
    struct wlr_scene_tree *tree;
    int width;
    int height;
};

typedef struct render_surface_t render_surface_t;

render_surface_t *render_surface_create(struct wlr_scene_tree *parent, int width, int height) {
    (void)parent; (void)width; (void)height;
    return NULL;
}

void render_surface_destroy(render_surface_t *rs) {
    (void)rs;
}

void render_surface_resize(render_surface_t *rs, int width, int height) {
    (void)rs; (void)width; (void)height;
}

void render_surface_commit(render_surface_t *rs) {
    (void)rs;
}

void draw_titlebar(render_surface_t *rs, titlebar_config_t *config,
                   const char *title, bool active, int hover_button) {
    (void)rs; (void)config; (void)title; (void)active; (void)hover_button;
}

void visual_init(theme_preset_t preset) {
    (void)preset;
}

int visual_load_config(const char *path) {
    (void)path;
    return 0;
}

visual_config_t *visual_get_config(void) {
    return NULL;
}

void visual_set_theme(theme_preset_t preset) {
    (void)preset;
}

void visual_cleanup(void) {
}
