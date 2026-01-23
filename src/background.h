#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <stdint.h>
#include <stdbool.h>
#include <wayland-server-core.h>

struct wlr_scene_tree;
struct wlr_scene_buffer;
struct background_config;

struct wlr_scene_buffer *background_create(struct wlr_scene_tree *parent,
                                           int width, int height,
                                           struct background_config *bg);

void background_update(struct wlr_scene_buffer *scene_buf,
                      int width, int height,
                      struct background_config *bg);

#endif
