/* decor_visual.h - Visual system integration header (fixed) */
#ifndef DECOR_VISUAL_H
#define DECOR_VISUAL_H

#include "visual_types.h"
#include "titlebar_render.h"
#include "core.h"

/* Initialize visual system with a theme preset */
void visual_init(theme_preset_t preset);

/* Load visual configuration from TOML file */
int visual_load_config(const char *path);

/* Get current visual configuration */
visual_config_t *visual_get_config(void);

/* Apply a theme preset */
void visual_set_theme(theme_preset_t preset);

/* Create decoration using visual system */
void decor_create_visual(struct toplevel *toplevel);

/* Update decoration (handles focus state, title changes) */
void decor_update_visual(struct toplevel *toplevel, bool focused);

/* Update decoration size */
void decor_set_size_visual(struct toplevel *toplevel, int width);

/* Destroy decoration */
void decor_destroy_visual(struct toplevel *toplevel);

/* Hit test for decoration elements */
enum decor_hit decor_hit_test_visual(struct toplevel *toplevel, double lx, double ly);

/* Update hover state for interactive elements */
void decor_update_hover_visual(struct toplevel *toplevel, double lx, double ly);

/* Cleanup visual system resources */
void visual_cleanup(void);

#endif /* DECOR_VISUAL_H */
