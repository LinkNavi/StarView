#ifndef VISUAL_H
#define VISUAL_H

#include "visual_types.h"

/* Stub render surface type */
typedef struct render_surface_t render_surface_t;

/* Stub functions */
render_surface_t *render_surface_create(struct wlr_scene_tree *parent, int width, int height);
void render_surface_destroy(render_surface_t *rs);
void render_surface_resize(render_surface_t *rs, int width, int height);
void render_surface_commit(render_surface_t *rs);
void draw_titlebar(render_surface_t *rs, titlebar_config_t *config, 
                   const char *title, bool active, int hover_button);

/* Visual system initialization */
void visual_init(theme_preset_t preset);
int visual_load_config(const char *path);
visual_config_t *visual_get_config(void);
void visual_set_theme(theme_preset_t preset);
void visual_cleanup(void);

#endif
