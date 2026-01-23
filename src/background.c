#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "background.h"
#include "config.h"
#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <drm_fourcc.h>

struct cairo_buffer {
    struct wlr_buffer base;
    cairo_surface_t *surface;
    void *data;
    int stride;
};

static void cairo_buffer_destroy(struct wlr_buffer *wlr_buffer) {
    struct cairo_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
    cairo_surface_destroy(buffer->surface);
    free(buffer->data);
    free(buffer);
}

static bool cairo_buffer_begin_data_ptr_access(struct wlr_buffer *wlr_buffer,
                                                uint32_t flags, void **data,
                                                uint32_t *format, size_t *stride) {
    (void)flags;
    struct cairo_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
    *data = buffer->data;
    *format = DRM_FORMAT_ARGB8888;
    *stride = buffer->stride;
    return true;
}

static void cairo_buffer_end_data_ptr_access(struct wlr_buffer *wlr_buffer) {
    (void)wlr_buffer;
}

static const struct wlr_buffer_impl cairo_buffer_impl = {
    .destroy = cairo_buffer_destroy,
    .begin_data_ptr_access = cairo_buffer_begin_data_ptr_access,
    .end_data_ptr_access = cairo_buffer_end_data_ptr_access,
};

static struct cairo_buffer *cairo_buffer_create(int width, int height) {
    struct cairo_buffer *buffer = calloc(1, sizeof(*buffer));
    if (!buffer) return NULL;

    buffer->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    buffer->data = calloc(1, buffer->stride * height);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }

    buffer->surface = cairo_image_surface_create_for_data(
        buffer->data, CAIRO_FORMAT_ARGB32, width, height, buffer->stride);

    if (cairo_surface_status(buffer->surface) != CAIRO_STATUS_SUCCESS) {
        free(buffer->data);
        free(buffer);
        return NULL;
    }

    wlr_buffer_init(&buffer->base, &cairo_buffer_impl, width, height);
    return buffer;
}


static cairo_surface_t *load_image(const char *path) {
    fprintf(stderr, "[BG] Attempting to load image: %s\n", path);
    

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[BG] ERROR: Cannot open file: %s\n", path);
        return NULL;
    }
    fclose(f);
    
    cairo_surface_t *image = cairo_image_surface_create_from_png(path);
    cairo_status_t status = cairo_surface_status(image);
    
    if (status != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "[BG] ERROR: Cairo failed to load PNG: %s (error: %s)\n", 
                path, cairo_status_to_string(status));
        cairo_surface_destroy(image);
        return NULL;
    }
    
    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);
    fprintf(stderr, "[BG] Successfully loaded image: %dx%d\n", w, h);
    
    return image;
}

static void draw_image_fill(cairo_t *cr, cairo_surface_t *image, 
                           int width, int height) {
    int img_w = cairo_image_surface_get_width(image);
    int img_h = cairo_image_surface_get_height(image);
    
    double scale_x = (double)width / img_w;
    double scale_y = (double)height / img_h;
    double scale = (scale_x > scale_y) ? scale_x : scale_y;
    
    double offset_x = (width - img_w * scale) / 2;
    double offset_y = (height - img_h * scale) / 2;
    
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
}

static void draw_image_fit(cairo_t *cr, cairo_surface_t *image,
                           int width, int height) {
    int img_w = cairo_image_surface_get_width(image);
    int img_h = cairo_image_surface_get_height(image);
    
    double scale_x = (double)width / img_w;
    double scale_y = (double)height / img_h;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    double offset_x = (width - img_w * scale) / 2;
    double offset_y = (height - img_h * scale) / 2;
    
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
}

static void draw_image_center(cairo_t *cr, cairo_surface_t *image,
                              int width, int height) {
    int img_w = cairo_image_surface_get_width(image);
    int img_h = cairo_image_surface_get_height(image);
    
    double offset_x = (width - img_w) / 2;
    double offset_y = (height - img_h) / 2;
    
    cairo_set_source_surface(cr, image, offset_x, offset_y);
    cairo_paint(cr);
}

static void draw_image_stretch(cairo_t *cr, cairo_surface_t *image,
                               int width, int height) {
    int img_w = cairo_image_surface_get_width(image);
    int img_h = cairo_image_surface_get_height(image);
    
    double scale_x = (double)width / img_w;
    double scale_y = (double)height / img_h;
    
    cairo_scale(cr, scale_x, scale_y);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
}

static void draw_image_tile(cairo_t *cr, cairo_surface_t *image,
                            int width, int height) {
    cairo_pattern_t *pattern = cairo_pattern_create_for_surface(image);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
    cairo_set_source(cr, pattern);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pattern);
}

struct wlr_scene_buffer *background_create(struct wlr_scene_tree *parent,
                                           int width, int height,
                                           struct background_config *bg) {
    if (!bg || !parent) return NULL;

    fprintf(stderr, "[BG] Creating background %dx%d, enabled=%d, image_path='%s'\n",
            width, height, bg->enabled, bg->image_path);

    struct cairo_buffer *buffer = cairo_buffer_create(width, height);
    if (!buffer) return NULL;

    cairo_t *cr = cairo_create(buffer->surface);
    
    /* Always draw background color first */
    cairo_set_source_rgba(cr, 
        ((bg->color >> 24) & 0xff) / 255.0,
        ((bg->color >> 16) & 0xff) / 255.0,
        ((bg->color >> 8) & 0xff) / 255.0,
        (bg->color & 0xff) / 255.0);
    cairo_paint(cr);

    /* Draw image if path exists and is not empty */
    if (bg->image_path[0] != '\0') {
        fprintf(stderr, "[BG] Image path not empty, attempting load\n");
        cairo_surface_t *image = load_image(bg->image_path);
        if (image) {
            fprintf(stderr, "[BG] Drawing image with mode %d\n", bg->mode);
            switch (bg->mode) {
            case BG_FILL:
                draw_image_fill(cr, image, width, height);
                break;
            case BG_FIT:
                draw_image_fit(cr, image, width, height);
                break;
            case BG_CENTER:
                draw_image_center(cr, image, width, height);
                break;
            case BG_STRETCH:
                draw_image_stretch(cr, image, width, height);
                break;
            case BG_TILE:
                draw_image_tile(cr, image, width, height);
                break;
            case BG_COLOR:
            default:
                break;
            }
            cairo_surface_destroy(image);
        } else {
            fprintf(stderr, "[BG] Failed to load image\n");
        }
    } else {
        fprintf(stderr, "[BG] No image path specified\n");
    }

    cairo_destroy(cr);
    cairo_surface_flush(buffer->surface);

    struct wlr_scene_buffer *scene_buf = wlr_scene_buffer_create(parent, &buffer->base);
    wlr_buffer_drop(&buffer->base);
    
    if (scene_buf) {
        wlr_scene_node_set_position(&scene_buf->node, 0, 0);
        wlr_scene_node_lower_to_bottom(&scene_buf->node);
    }
    
    return scene_buf;
}

void background_update(struct wlr_scene_buffer *scene_buf,
                      int width, int height,
                      struct background_config *bg) {
    if (!scene_buf || !bg) return;

    struct cairo_buffer *buffer = cairo_buffer_create(width, height);
    if (!buffer) return;

    cairo_t *cr = cairo_create(buffer->surface);
    
    cairo_set_source_rgba(cr,
        ((bg->color >> 24) & 0xff) / 255.0,
        ((bg->color >> 16) & 0xff) / 255.0,
        ((bg->color >> 8) & 0xff) / 255.0,
        (bg->color & 0xff) / 255.0);
    cairo_paint(cr);

    if (bg->image_path[0] != '\0') {
        cairo_surface_t *image = load_image(bg->image_path);
        if (image) {
            switch (bg->mode) {
            case BG_FILL:
                draw_image_fill(cr, image, width, height);
                break;
            case BG_FIT:
                draw_image_fit(cr, image, width, height);
                break;
            case BG_CENTER:
                draw_image_center(cr, image, width, height);
                break;
            case BG_STRETCH:
                draw_image_stretch(cr, image, width, height);
                break;
            case BG_TILE:
                draw_image_tile(cr, image, width, height);
                break;
            case BG_COLOR:
            default:
                break;
            }
            cairo_surface_destroy(image);
        }
    }

    cairo_destroy(cr);
    cairo_surface_flush(buffer->surface);

    wlr_scene_buffer_set_buffer(scene_buf, &buffer->base);
    wlr_buffer_drop(&buffer->base);
}
