/* visual_config_stub.c - Stub implementations to fix compilation */
#define _POSIX_C_SOURCE 200809L

#include "../visual_types.h"
#include "../titlebar_render.h"
#include <stdlib.h>
#include <string.h>

/* Stub function - returns empty config */
visual_config_t visual_config_default(void);
visual_config_t visual_config_preset(theme_preset_t preset);

/* Stub load function - does nothing */
int visual_config_load(visual_config_t *config, const char *path) {
    (void)config;
    (void)path;
    return 0;
}

/* Stub save function - does nothing */
int visual_config_save(visual_config_t *config, const char *path) {
    (void)config;
    (void)path;
    return 0;
}
