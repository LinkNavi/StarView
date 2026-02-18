

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "titlebar_render.h"
#include "config.h"
#include <cairo/cairo.h>
#include <drm_fourcc.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_scene.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifdef USE_TOML
#include "toml.h"
#endif

/* Global theme */
static struct titlebar_theme *global_theme = NULL;

/* ============================================================================
 * Buffer implementation for Cairo surfaces
 * ============================================================================
 */

struct decor_config; // Forward declaration
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
                                               uint32_t *format,
                                               size_t *stride) {
  struct cairo_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
  *data = buffer->data;
  *format = DRM_FORMAT_ARGB8888;
  *stride = buffer->stride;
  return true;
}

static void cairo_buffer_end_data_ptr_access(struct wlr_buffer *wlr_buffer) {
  /* Nothing to do */
}

static const struct wlr_buffer_impl cairo_buffer_impl = {
    .destroy = cairo_buffer_destroy,
    .begin_data_ptr_access = cairo_buffer_begin_data_ptr_access,
    .end_data_ptr_access = cairo_buffer_end_data_ptr_access,
};

static struct cairo_buffer *cairo_buffer_create(int width, int height) {
  struct cairo_buffer *buffer = calloc(1, sizeof(*buffer));
  if (!buffer)
    return NULL;

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

/* ============================================================================
 * Image loading helpers
 * ============================================================================
 */

static cairo_surface_t *load_png_image(const char *path) {
    if (!path || !path[0]) {
        return NULL;
    }
    
    // Expand ~ to home directory
    char expanded_path[1024];
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(expanded_path, sizeof(expanded_path), "%s%s", home, path + 1);
            path = expanded_path;
        }
    }
    
    fprintf(stderr, "[TITLEBAR] Loading PNG: %s\n", path);
    
    cairo_surface_t *img = cairo_image_surface_create_from_png(path);
    cairo_status_t status = cairo_surface_status(img);
    
    if (status != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "[TITLEBAR] PNG load failed: %s\n", cairo_status_to_string(status));
        cairo_surface_destroy(img);
        return NULL;
    }
    
    int w = cairo_image_surface_get_width(img);
    int h = cairo_image_surface_get_height(img);
    fprintf(stderr, "[TITLEBAR] Loaded: %dx%d\n", w, h);
    
    return img;
}

static void draw_custom_icon(cairo_t *cr, const char *path, 
                            double x, double y, double size) {
    if (!path || !path[0]) return;
    
    cairo_surface_t *img = load_png_image(path);
    if (!img) return;
    
    int img_w = cairo_image_surface_get_width(img);
    int img_h = cairo_image_surface_get_height(img);
    
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_scale(cr, size / img_w, size / img_h);
    cairo_set_source_surface(cr, img, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    
    cairo_surface_destroy(img);
}

static void draw_image_background(cairo_t *cr, const char *path, bool tile,
                                  int width, int height) {
    fprintf(stderr, "[TITLEBAR] draw_image_background: path='%s', tile=%d, size=%dx%d\n", 
            path, tile, width, height);
    
    cairo_surface_t *img = load_png_image(path);
    if (!img) {
        fprintf(stderr, "[TITLEBAR] draw_image_background: failed to load image\n");
        return;
    }
    
    int img_w = cairo_image_surface_get_width(img);
    int img_h = cairo_image_surface_get_height(img);
    
    if (tile) {
        // Tile the image
        cairo_pattern_t *pattern = cairo_pattern_create_for_surface(img);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        cairo_set_source(cr, pattern);
        cairo_rectangle(cr, 0, 0, width, height);
        cairo_fill(cr);
        cairo_pattern_destroy(pattern);
        fprintf(stderr, "[TITLEBAR] Tiled background image\n");
    } else {
        // Stretch to fit
        cairo_save(cr);
        cairo_scale(cr, (double)width / img_w, (double)height / img_h);
        cairo_set_source_surface(cr, img, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
        fprintf(stderr, "[TITLEBAR] Stretched background image\n");
    }
    
    cairo_surface_destroy(img);
}

/* ============================================================================
 * Color helpers
 * ============================================================================
 */

struct color color_from_hex(uint32_t hex) {
  return (struct color){
      .r = ((hex >> 24) & 0xff) / 255.0f,
      .g = ((hex >> 16) & 0xff) / 255.0f,
      .b = ((hex >> 8) & 0xff) / 255.0f,
      .a = (hex & 0xff) / 255.0f,
  };
}

struct color color_from_rgba(float r, float g, float b, float a) {
  return (struct color){.r = r, .g = g, .b = b, .a = a};
}

uint32_t color_to_hex(struct color c) {
  return ((uint32_t)(c.r * 255) << 24) | ((uint32_t)(c.g * 255) << 16) |
         ((uint32_t)(c.b * 255) << 8) | (uint32_t)(c.a * 255);
}

struct color color_blend(struct color a, struct color b, float t) {
  return (struct color){
      .r = a.r + (b.r - a.r) * t,
      .g = a.g + (b.g - a.g) * t,
      .b = a.b + (b.b - a.b) * t,
      .a = a.a + (b.a - a.a) * t,
  };
}

struct color color_lighten(struct color c, float amount) {
  return (struct color){
      .r = fminf(1.0f, c.r + amount),
      .g = fminf(1.0f, c.g + amount),
      .b = fminf(1.0f, c.b + amount),
      .a = c.a,
  };
}

struct color color_darken(struct color c, float amount) {
  return (struct color){
      .r = fmaxf(0.0f, c.r - amount),
      .g = fmaxf(0.0f, c.g - amount),
      .b = fmaxf(0.0f, c.b - amount),
      .a = c.a,
  };
}

static void cairo_set_color(cairo_t *cr, struct color c) {
  cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a);
}

/* ============================================================================
 * Drawing primitives
 * ============================================================================
 */

static void draw_rounded_rect(cairo_t *cr, double x, double y, double w,
                              double h, double r) {
  if (r <= 0) {
    cairo_rectangle(cr, x, y, w, h);
    return;
  }

  r = fmin(r, fmin(w / 2, h / 2));

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
  cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
  cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
  cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
  cairo_close_path(cr);
}

static void draw_rounded_rect_top(cairo_t *cr, double x, double y, double w,
                                  double h, double r) {
  if (r <= 0) {
    cairo_rectangle(cr, x, y, w, h);
    return;
  }

  r = fmin(r, fmin(w / 2, h));

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
  cairo_line_to(cr, x + w, y + h);
  cairo_line_to(cr, x, y + h);
  cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
  cairo_close_path(cr);
}

static void draw_circle(cairo_t *cr, double cx, double cy, double r) {
  cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
}

static void apply_gradient(cairo_t *cr, struct gradient *grad, double x,
                           double y, double w, double h) {
  if (grad->direction == GRADIENT_NONE)
    return;

  cairo_pattern_t *pattern;

  switch (grad->direction) {
  case GRADIENT_VERTICAL:
    pattern = cairo_pattern_create_linear(x, y, x, y + h);
    break;
  case GRADIENT_HORIZONTAL:
    pattern = cairo_pattern_create_linear(x, y, x + w, y);
    break;
  case GRADIENT_DIAGONAL:
    pattern = cairo_pattern_create_linear(x, y, x + w, y + h);
    break;
  default:
    return;
  }

  cairo_pattern_add_color_stop_rgba(pattern, 0, grad->start.r, grad->start.g,
                                    grad->start.b, grad->start.a);
  cairo_pattern_add_color_stop_rgba(pattern, 1, grad->end.r, grad->end.g,
                                    grad->end.b, grad->end.a);

  cairo_set_source(cr, pattern);
  cairo_pattern_destroy(pattern);
}

static void draw_shadow(cairo_t *cr, double x, double y, double w, double h,
                        double radius, struct shadow_style *shadow) {
  if (!shadow->enabled)
    return;

  // Multiple passes for blur approximation
  int steps = (int)(shadow->blur / 2) + 1;
  for (int i = steps; i > 0; i--) {
    float alpha = shadow->color.a * (1.0f - (float)i / (steps + 1)) * 0.3f;
    cairo_set_source_rgba(cr, shadow->color.r, shadow->color.g, shadow->color.b,
                          alpha);

    double offset = (double)i * 0.5;
    draw_rounded_rect(cr, x + shadow->offset_x - offset,
                      y + shadow->offset_y - offset, w + offset * 2,
                      h + offset * 2, radius + offset);
    cairo_fill(cr);
  }
}

/* ============================================================================
 * Button icon drawing
 * ============================================================================
 */

static void draw_close_icon(cairo_t *cr, double cx, double cy, double size,
                            struct color color) {
  cairo_set_color(cr, color);
  cairo_set_line_width(cr, 1.5);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  double d = size * 0.3;
  cairo_move_to(cr, cx - d, cy - d);
  cairo_line_to(cr, cx + d, cy + d);
  cairo_move_to(cr, cx + d, cy - d);
  cairo_line_to(cr, cx - d, cy + d);
  cairo_stroke(cr);
}

static void draw_maximize_icon(cairo_t *cr, double cx, double cy, double size,
                               struct color color) {
  cairo_set_color(cr, color);
  cairo_set_line_width(cr, 1.5);

  double d = size * 0.3;
  cairo_rectangle(cr, cx - d, cy - d, d * 2, d * 2);
  cairo_stroke(cr);
}

static void draw_minimize_icon(cairo_t *cr, double cx, double cy, double size,
                               struct color color) {
  cairo_set_color(cr, color);
  cairo_set_line_width(cr, 2.0);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  double d = size * 0.3;
  cairo_move_to(cr, cx - d, cy);
  cairo_line_to(cr, cx + d, cy);
  cairo_stroke(cr);
}

/* ============================================================================
 * Button rendering
 * ============================================================================
 */

static void render_button(cairo_t *cr, double x, double y,
                          struct button_theme *theme,
                          struct button_style *style, enum button_type type) {
  double w = theme->width;
  double h = theme->height;
  double cx = x + w / 2;
  double cy = y + h / 2;

  /* Draw shadow */
  if (theme->shape == SHAPE_CIRCLE) {
    draw_shadow(cr, x, y, w, h, w / 2, &style->shadow);
  } else {
    draw_shadow(cr, x, y, w, h, style->border.radius, &style->shadow);
  }

  /* Draw background */
  if (style->bg_gradient.direction != GRADIENT_NONE) {
    apply_gradient(cr, &style->bg_gradient, x, y, w, h);
  } else {
    cairo_set_color(cr, style->bg_color);
  }

  switch (theme->shape) {
  case SHAPE_CIRCLE:
    draw_circle(cr, cx, cy, w / 2);
    break;
  case SHAPE_ROUNDED_RECT:
    draw_rounded_rect(cr, x, y, w, h, style->border.radius);
    break;
  case SHAPE_PILL:
    draw_rounded_rect(cr, x, y, w, h, h / 2);
    break;
  default:
    cairo_rectangle(cr, x, y, w, h);
    break;
  }
  cairo_fill(cr);

  /* Draw border */
  if (style->border.width > 0) {
    cairo_set_color(cr, style->border.color);
    cairo_set_line_width(cr, style->border.width);

    switch (theme->shape) {
    case SHAPE_CIRCLE:
      draw_circle(cr, cx, cy, w / 2 - style->border.width / 2);
      break;
    case SHAPE_ROUNDED_RECT:
      draw_rounded_rect(cr, x, y, w, h, style->border.radius);
      break;
    case SHAPE_PILL:
      draw_rounded_rect(cr, x, y, w, h, h / 2);
      break;
    default:
      cairo_rectangle(cr, x, y, w, h);
      break;
    }
    cairo_stroke(cr);
  }

  /* Draw icon - try custom first, fallback to drawn */
  double icon_size = fmin(w, h) * style->icon_scale;
  bool drew_custom = false;

  switch (type) {
  case BTN_TYPE_CLOSE:
    if (config.decor.icon_close_path[0]) {
        cairo_save(cr);
        draw_custom_icon(cr, config.decor.icon_close_path, 
                        cx - icon_size/2, cy - icon_size/2, icon_size);
        cairo_restore(cr);
        drew_custom = true;
    }
    if (!drew_custom) {
        draw_close_icon(cr, cx, cy, icon_size, style->icon_color);
    }
    break;
    
  case BTN_TYPE_MAXIMIZE:
    if (config.decor.icon_maximize_path[0]) {
        cairo_save(cr);
        draw_custom_icon(cr, config.decor.icon_maximize_path,
                        cx - icon_size/2, cy - icon_size/2, icon_size);
        cairo_restore(cr);
        drew_custom = true;
    }
    if (!drew_custom) {
        draw_maximize_icon(cr, cx, cy, icon_size, style->icon_color);
    }
    break;
    
  case BTN_TYPE_MINIMIZE:
    if (config.decor.icon_minimize_path[0]) {
        cairo_save(cr);
        draw_custom_icon(cr, config.decor.icon_minimize_path,
                        cx - icon_size/2, cy - icon_size/2, icon_size);
        cairo_restore(cr);
        drew_custom = true;
    }
    if (!drew_custom) {
        draw_minimize_icon(cr, cx, cy, icon_size, style->icon_color);
    }
    break;
    
  default:
    break;
  }
}

/* ============================================================================
 * Theme presets
 * ============================================================================
 */

struct titlebar_theme *titlebar_theme_create(void) {
  struct titlebar_theme *theme = calloc(1, sizeof(*theme));
  if (!theme)
    return NULL;

  /* Load default preset */
  titlebar_theme_load_preset(theme, THEME_PRESET_DEFAULT);

  return theme;
}

void titlebar_theme_destroy(struct titlebar_theme *theme) {
  if (!theme)
    return;
  free(theme->extra_buttons);
  free(theme);
}

void titlebar_theme_load_preset(struct titlebar_theme *theme,
                                enum theme_preset preset) {
  memset(theme, 0, sizeof(*theme));

  switch (preset) {
  case THEME_PRESET_MACOS:
    /* macOS Big Sur style - refined traffic lights */
    theme->height = 38;
    theme->padding_left = 12;
    theme->padding_right = 12;
    theme->corner_radius_top = 12;

    /* Modern macOS gradient */
    theme->bg_color = color_from_hex(0xebebedff);
    theme->bg_color_inactive = color_from_hex(0xf6f6f7ff);
    theme->bg_gradient.direction = GRADIENT_VERTICAL;
    theme->bg_gradient.start = color_from_hex(0xf5f5f6ff);
    theme->bg_gradient.end = color_from_hex(0xe3e3e5ff);

    /* Subtle border */
    theme->border.width = 0.5f;
    theme->border.color = color_from_hex(0xc9c9cbff);
    theme->border.radius = 12;

    /* Soft shadow */
    theme->shadow.enabled = true;
    theme->shadow.offset_x = 0;
    theme->shadow.offset_y = 2;
    theme->shadow.blur = 8;
    theme->shadow.color = color_from_hex(0x00000020);

    /* SF Pro Display font */
    theme->title.font_size = 13;
    strcpy(theme->title.font_family,
           "SF Pro Display, -apple-system, sans-serif");
    theme->title.font_weight = 600;
    theme->title.color = color_from_hex(0x000000ff);
    theme->title_inactive.color = color_from_hex(0x00000060);
    theme->title_align = ALIGN_CENTER;

    theme->buttons_left = true;
    theme->button_spacing = 8;
    theme->button_margin = 12;

    /* Refined traffic light buttons */
    theme->btn_close.type = BTN_TYPE_CLOSE;
    theme->btn_close.shape = SHAPE_CIRCLE;
    theme->btn_close.width = 13;
    theme->btn_close.height = 13;

    theme->btn_close.normal.bg_color = color_from_hex(0xfc615dff);
    theme->btn_close.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_close.normal.icon_scale = 0.0f;

    theme->btn_close.hover.bg_color = color_from_hex(0xfc615dff);
    theme->btn_close.hover.icon_color = color_from_hex(0x4a0807ff);
    theme->btn_close.hover.icon_scale = 0.7f;
    theme->btn_close.hover.border.width = 0.5f;
    theme->btn_close.hover.border.color = color_from_hex(0xca4542ff);

    theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
    theme->btn_minimize.shape = SHAPE_CIRCLE;
    theme->btn_minimize.width = 13;
    theme->btn_minimize.height = 13;

    theme->btn_minimize.normal.bg_color = color_from_hex(0xfdbe40ff);
    theme->btn_minimize.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_minimize.normal.icon_scale = 0.0f;

    theme->btn_minimize.hover.bg_color = color_from_hex(0xfdbe40ff);
    theme->btn_minimize.hover.icon_color = color_from_hex(0x734e00ff);
    theme->btn_minimize.hover.icon_scale = 0.65f;

    theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
    theme->btn_maximize.shape = SHAPE_CIRCLE;
    theme->btn_maximize.width = 13;
    theme->btn_maximize.height = 13;

    theme->btn_maximize.normal.bg_color = color_from_hex(0x34c748ff);
    theme->btn_maximize.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_maximize.normal.icon_scale = 0.0f;

    theme->btn_maximize.hover.bg_color = color_from_hex(0x34c748ff);
    theme->btn_maximize.hover.icon_color = color_from_hex(0x0e5017ff);
    theme->btn_maximize.hover.icon_scale = 0.65f;
    break;

  case THEME_PRESET_WINDOWS:
    /* Windows 11 Fluent Design */
    theme->height = 32;
    theme->padding_left = 12;
    theme->padding_right = 0;
    theme->corner_radius_top = 8;

    /* Mica material effect */
    theme->bg_color = color_from_hex(0xf3f3f3fa);
    theme->bg_color_inactive = color_from_hex(0xfafafafe);

    /* Title */
    theme->title.font_size = 12;
    strcpy(theme->title.font_family, "Segoe UI Variable, Segoe UI, sans-serif");
    theme->title.font_weight = 600;
    theme->title.color = color_from_hex(0x000000e6);
    theme->title_inactive.color = color_from_hex(0x00000066);
    theme->title_align = ALIGN_LEFT;

    theme->buttons_left = false;
    theme->button_spacing = 0;
    theme->button_margin = 0;

    /* Modern Windows buttons */
    theme->btn_close.shape = SHAPE_RECT;
    theme->btn_close.width = 46;
    theme->btn_close.height = 32;
    theme->btn_close.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_close.normal.icon_color = color_from_hex(0x000000e6);
    theme->btn_close.normal.icon_scale = 0.55f;

    theme->btn_close.hover.bg_color = color_from_hex(0xc42b1cff);
    theme->btn_close.hover.icon_color = color_from_hex(0xffffffff);
    theme->btn_close.hover.icon_scale = 0.55f;

    theme->btn_maximize.shape = SHAPE_RECT;
    theme->btn_maximize.width = 46;
    theme->btn_maximize.height = 32;
    theme->btn_maximize.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_maximize.normal.icon_color = color_from_hex(0x000000e6);
    theme->btn_maximize.normal.icon_scale = 0.5f;

    theme->btn_maximize.hover.bg_color = color_from_hex(0x0000000d);
    theme->btn_maximize.hover.icon_color = color_from_hex(0x000000ff);
    theme->btn_maximize.hover.icon_scale = 0.5f;

    theme->btn_minimize.shape = SHAPE_RECT;
    theme->btn_minimize.width = 46;
    theme->btn_minimize.height = 32;
    theme->btn_minimize.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_minimize.normal.icon_color = color_from_hex(0x000000e6);
    theme->btn_minimize.normal.icon_scale = 0.5f;

    theme->btn_minimize.hover.bg_color = color_from_hex(0x0000000d);
    theme->btn_minimize.hover.icon_color = color_from_hex(0x000000ff);
    theme->btn_minimize.hover.icon_scale = 0.5f;
    break;

  case THEME_PRESET_GNOME:
    /* GNOME 45+ Libadwaita style */
    theme->height = 40;
    theme->padding_left = 12;
    theme->padding_right = 12;
    theme->corner_radius_top = 12;

    theme->bg_color = color_from_hex(0x303030ff);
    theme->bg_color_inactive = color_from_hex(0x242424ff);

    /* Subtle gradient */
    theme->bg_gradient.direction = GRADIENT_VERTICAL;
    theme->bg_gradient.start = color_from_hex(0x383838ff);
    theme->bg_gradient.end = color_from_hex(0x2a2a2aff);

    /* Border */
    theme->border.width = 1;
    theme->border.color = color_from_hex(0x00000040);
    theme->border.radius = 12;

    /* Title */
    theme->title.font_size = 11;
    strcpy(theme->title.font_family, "Cantarell, Inter, sans-serif");
    theme->title.font_weight = 700;
    theme->title.color = color_from_hex(0xfffffffa);
    theme->title_inactive.color = color_from_hex(0xffffff99);
    theme->title_align = ALIGN_CENTER;

    theme->buttons_left = false;
    theme->button_spacing = 6;
    theme->button_margin = 6;

    /* Modern GNOME buttons */
    theme->btn_close.shape = SHAPE_CIRCLE;
    theme->btn_close.width = 22;
    theme->btn_close.height = 22;

    theme->btn_close.normal.bg_color = color_from_hex(0xffffff1a);
    theme->btn_close.normal.icon_color = color_from_hex(0xfffffffa);
    theme->btn_close.normal.icon_scale = 0.6f;

    theme->btn_close.hover.bg_color = color_from_hex(0xff6b6bff);
    theme->btn_close.hover.icon_color = color_from_hex(0xffffffff);
    theme->btn_close.hover.icon_scale = 0.6f;

    theme->btn_maximize.shape = SHAPE_CIRCLE;
    theme->btn_maximize.width = 22;
    theme->btn_maximize.height = 22;

    theme->btn_maximize.normal.bg_color = color_from_hex(0xffffff1a);
    theme->btn_maximize.normal.icon_color = color_from_hex(0xfffffffa);
    theme->btn_maximize.normal.icon_scale = 0.55f;

    theme->btn_maximize.hover.bg_color = color_from_hex(0xffffff2a);
    theme->btn_maximize.hover.icon_color = color_from_hex(0xffffffff);
    theme->btn_maximize.hover.icon_scale = 0.55f;

    theme->btn_minimize.shape = SHAPE_CIRCLE;
    theme->btn_minimize.width = 22;
    theme->btn_minimize.height = 22;

    theme->btn_minimize.normal.bg_color = color_from_hex(0xffffff1a);
    theme->btn_minimize.normal.icon_color = color_from_hex(0xfffffffa);
    theme->btn_minimize.normal.icon_scale = 0.55f;

    theme->btn_minimize.hover.bg_color = color_from_hex(0xffffff2a);
    theme->btn_minimize.hover.icon_color = color_from_hex(0xffffffff);
    theme->btn_minimize.hover.icon_scale = 0.55f;
    break;

  case THEME_PRESET_MINIMAL:
    /* Ultra-minimal design */
    theme->height = 28;
    theme->padding_left = 8;
    theme->padding_right = 8;
    theme->corner_radius_top = 0;

    theme->bg_color = color_from_hex(0x1a1b26ff);
    theme->bg_color_inactive = color_from_hex(0x16161eff);

    theme->title.font_size = 10;
    strcpy(theme->title.font_family, "JetBrains Mono, monospace");
    theme->title.font_weight = 500;
    theme->title.color = color_from_hex(0xa9b1d6ff);
    theme->title_inactive.color = color_from_hex(0x565f89ff);
    theme->title_align = ALIGN_LEFT;

    theme->buttons_left = false;
    theme->button_spacing = 6;
    theme->button_margin = 6;

    /* Minimal flat buttons */
    theme->btn_close.shape = SHAPE_RECT;
    theme->btn_close.width = 18;
    theme->btn_close.height = 18;

    theme->btn_close.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_close.normal.icon_color = color_from_hex(0xf7768eff);
    theme->btn_close.normal.icon_scale = 0.5f;

    theme->btn_close.hover.bg_color = color_from_hex(0xf7768e20);
    theme->btn_close.hover.icon_color = color_from_hex(0xf7768eff);
    theme->btn_close.hover.icon_scale = 0.5f;

    theme->btn_maximize.shape = SHAPE_RECT;
    theme->btn_maximize.width = 18;
    theme->btn_maximize.height = 18;

    theme->btn_maximize.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_maximize.normal.icon_color = color_from_hex(0x9ece6aff);
    theme->btn_maximize.normal.icon_scale = 0.45f;

    theme->btn_maximize.hover.bg_color = color_from_hex(0x9ece6a20);
    theme->btn_maximize.hover.icon_color = color_from_hex(0x9ece6aff);
    theme->btn_maximize.hover.icon_scale = 0.45f;

    theme->btn_minimize.shape = SHAPE_RECT;
    theme->btn_minimize.width = 18;
    theme->btn_minimize.height = 18;

    theme->btn_minimize.normal.bg_color = color_from_hex(0x00000000);
    theme->btn_minimize.normal.icon_color = color_from_hex(0xe0af68ff);
    theme->btn_minimize.normal.icon_scale = 0.45f;

    theme->btn_minimize.hover.bg_color = color_from_hex(0xe0af6820);
    theme->btn_minimize.hover.icon_color = color_from_hex(0xe0af68ff);
    theme->btn_minimize.hover.icon_scale = 0.45f;
    break;

  case THEME_PRESET_DEFAULT:
  default:
    /* Modern Catppuccin Mocha - enhanced */
    theme->height = 34;
    theme->padding_left = 12;
    theme->padding_right = 12;
    theme->corner_radius_top = 10;

    theme->bg_color = color_from_hex(0x1e1e2eff);
    theme->bg_color_inactive = color_from_hex(0x181825ff);

    /* Smooth gradient */
    theme->bg_gradient.direction = GRADIENT_VERTICAL;
    theme->bg_gradient.start = color_from_hex(0x313244ff);
    theme->bg_gradient.end = color_from_hex(0x1e1e2eff);

    /* Subtle glow border */
    theme->border.width = 0;
    theme->shadow.enabled = true;
    theme->shadow.offset_x = 0;
    theme->shadow.offset_y = 0;
    theme->shadow.blur = 2;
    theme->shadow.color = color_from_hex(0x89b4fa30);

    theme->title.font_size = 12;
    strcpy(theme->title.font_family, "Inter, sans-serif");
    theme->title.font_weight = 600;
    theme->title.color = color_from_hex(0xcdd6f4ff);
    theme->title_inactive.color = color_from_hex(0x6c7086ff);
    theme->title_align = ALIGN_CENTER;

    theme->buttons_left = false;
    theme->button_spacing = 8;
    theme->button_margin = 10;

    /* Modern buttons with glow effect */
    theme->btn_close.type = BTN_TYPE_CLOSE;
    theme->btn_close.shape = SHAPE_CIRCLE;
    theme->btn_close.width = 16;
    theme->btn_close.height = 16;

    theme->btn_close.normal.bg_color = color_from_hex(0xf38ba8ff);
    theme->btn_close.normal.shadow.enabled = true;
    theme->btn_close.normal.shadow.blur = 4;
    theme->btn_close.normal.shadow.color = color_from_hex(0xf38ba840);
    theme->btn_close.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_close.normal.icon_scale = 0.0f;

    theme->btn_close.hover.bg_color = color_from_hex(0xeba0acff);
    theme->btn_close.hover.shadow.enabled = true;
    theme->btn_close.hover.shadow.blur = 8;
    theme->btn_close.hover.shadow.color = color_from_hex(0xf38ba860);
    theme->btn_close.hover.icon_color = color_from_hex(0x1e1e2eff);
    theme->btn_close.hover.icon_scale = 0.65f;

    theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
    theme->btn_maximize.shape = SHAPE_CIRCLE;
    theme->btn_maximize.width = 16;
    theme->btn_maximize.height = 16;

    theme->btn_maximize.normal.bg_color = color_from_hex(0xa6e3a1ff);
    theme->btn_maximize.normal.shadow.enabled = true;
    theme->btn_maximize.normal.shadow.blur = 4;
    theme->btn_maximize.normal.shadow.color = color_from_hex(0xa6e3a140);
    theme->btn_maximize.normal.icon_color = color_from_hex(0x00000000);

    theme->btn_maximize.hover.bg_color = color_from_hex(0x94e2d5ff);
    theme->btn_maximize.hover.shadow.enabled = true;
    theme->btn_maximize.hover.shadow.blur = 8;
    theme->btn_maximize.hover.shadow.color = color_from_hex(0xa6e3a160);
    theme->btn_maximize.hover.icon_color = color_from_hex(0x1e1e2eff);
    theme->btn_maximize.hover.icon_scale = 0.6f;

    theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
    theme->btn_minimize.shape = SHAPE_CIRCLE;
    theme->btn_minimize.width = 16;
    theme->btn_minimize.height = 16;

    theme->btn_minimize.normal.bg_color = color_from_hex(0xf9e2afff);
    theme->btn_minimize.normal.shadow.enabled = true;
    theme->btn_minimize.normal.shadow.blur = 4;
    theme->btn_minimize.normal.shadow.color = color_from_hex(0xf9e2af40);
    theme->btn_minimize.normal.icon_color = color_from_hex(0x00000000);

    theme->btn_minimize.hover.bg_color = color_from_hex(0xf5c2e7ff);
    theme->btn_minimize.hover.shadow.enabled = true;
    theme->btn_minimize.hover.shadow.blur = 8;
    theme->btn_minimize.hover.shadow.color = color_from_hex(0xf9e2af60);
    theme->btn_minimize.hover.icon_color = color_from_hex(0x1e1e2eff);
    theme->btn_minimize.hover.icon_scale = 0.6f;
    break;
  }

  theme->inactive_opacity = 0.85f;
}

/* ============================================================================
 * Titlebar rendering
 * ============================================================================
 */

static struct wlr_scene_buffer *
create_scene_buffer(struct wlr_scene_tree *parent,
                    struct cairo_buffer *buffer) {
  return wlr_scene_buffer_create(parent, &buffer->base);
}

static void render_titlebar_background(struct rendered_titlebar *tb,
                                       struct titlebar_theme *theme,
                                       bool active) {
  int w = tb->width;
  int h = theme->height;

  struct cairo_buffer *buffer = cairo_buffer_create(w, h);
  if (!buffer)
    return;

  cairo_t *cr = cairo_create(buffer->surface);

  /* Clear */
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* Draw shadow if enabled */
  draw_shadow(cr, 0, 0, w, h, theme->corner_radius_top, &theme->shadow);

  /* Draw background - check for custom image first */
  if (config.decor.bg_image_path[0]) {
    fprintf(stderr, "[TITLEBAR] Using custom background image: %s\n", config.decor.bg_image_path);
    draw_image_background(cr, config.decor.bg_image_path, 
                         config.decor.bg_image_tile, w, h);
  } else if (theme->bg_gradient.direction != GRADIENT_NONE && active) {
    fprintf(stderr, "[TITLEBAR] Using gradient background\n");
    apply_gradient(cr, &theme->bg_gradient, 0, 0, w, h);
  } else {
    fprintf(stderr, "[TITLEBAR] Using solid color background\n");
    cairo_set_color(cr, active ? theme->bg_color : theme->bg_color_inactive);
  }

  draw_rounded_rect_top(cr, 0, 0, w, h, theme->corner_radius_top);
  cairo_fill(cr);

  /* Draw border if enabled */
  if (theme->border.width > 0) {
    cairo_set_color(cr, theme->border.color);
    cairo_set_line_width(cr, theme->border.width);
    draw_rounded_rect_top(cr, 0, 0, w, h, theme->corner_radius_top);
    cairo_stroke(cr);
  }

  cairo_destroy(cr);
  cairo_surface_flush(buffer->surface);

  if (tb->background) {
    wlr_scene_node_destroy(&tb->background->node);
  }
  tb->background = create_scene_buffer(tb->tree, buffer);
  wlr_buffer_drop(&buffer->base);
}

struct wlr_scene_buffer *create_shadow_buffer(
    struct wlr_scene_tree *parent,
    int window_width,
    int window_height,
    struct shadow_config *shadow) {
    
    if (!shadow->enabled) return NULL;
    
    int w = window_width + (shadow->blur_radius * 2);
    int h = window_height + (shadow->blur_radius * 2) + shadow->offset_y;
    
    struct cairo_buffer *buffer = cairo_buffer_create(w, h);
    if (!buffer) return NULL;
    
    cairo_t *cr = cairo_create(buffer->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    // Convert shadow config to shadow_style for drawing
    struct shadow_style style = {
        .enabled = true,
        .blur = shadow->blur_radius,
        .offset_x = 0,
        .offset_y = 0,
        .color = color_from_hex(shadow->color)
    };
    style.color.a = shadow->opacity;
    
    int shadow_x = shadow->blur_radius;
    int shadow_y = shadow->blur_radius;
    draw_shadow(cr, shadow_x, shadow_y, window_width, window_height, 
                 config.decor.corner_radius, &style);
    
    cairo_destroy(cr);
    cairo_surface_flush(buffer->surface);
    
    struct wlr_scene_buffer *scene_buf = create_scene_buffer(parent, buffer);
    wlr_buffer_drop(&buffer->base);
    
    if (scene_buf) {
        wlr_scene_node_set_position(&scene_buf->node, 
                                    -shadow->blur_radius - shadow->offset_x,
                                    -shadow->blur_radius - shadow->offset_y);
    }
    
    return scene_buf;
}

static void render_title_text(struct rendered_titlebar *tb,
                              struct titlebar_theme *theme, const char *title,
                              bool active) {
  if (!title || !title[0])
    return;

  struct text_style *style = active ? &theme->title : &theme->title_inactive;

  /* Calculate available width for title */
  int btn_area = 0;
  if (theme->buttons_left) {
    btn_area = theme->button_margin +
               (theme->btn_close.width + theme->btn_minimize.width +
                theme->btn_maximize.width + theme->button_spacing * 2);
  } else {
    btn_area = theme->button_margin +
               (theme->btn_close.width + theme->btn_minimize.width +
                theme->btn_maximize.width + theme->button_spacing * 2);
  }

  int max_width =
      tb->width - theme->padding_left - theme->padding_right - btn_area - 20;
  if (max_width < 50)
    max_width = 50;

  /* Create buffer for text */
  struct cairo_buffer *buffer = cairo_buffer_create(max_width, theme->height);
  if (!buffer)
    return;

  cairo_t *cr = cairo_create(buffer->surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* Setup Pango - with custom font weight and style */
  PangoLayout *layout = pango_cairo_create_layout(cr);

  char font_desc[128];
  const char *weight_str = "Normal";
  if (config.decor.font_weight >= 700) weight_str = "Bold";
  else if (config.decor.font_weight >= 600) weight_str = "Semibold";
  else if (config.decor.font_weight >= 500) weight_str = "Medium";

  const char *style_str = config.decor.font_italic ? "Italic" : "";

  snprintf(font_desc, sizeof(font_desc), "%s %s %s %d", 
           style->font_family, weight_str, style_str, style->font_size);

  PangoFontDescription *font = pango_font_description_from_string(font_desc);
  pango_layout_set_font_description(layout, font);
  pango_font_description_free(font);

  pango_layout_set_text(layout, title, -1);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_width(layout, max_width * PANGO_SCALE);

  /* Get text dimensions */
  int text_width, text_height;
  pango_layout_get_pixel_size(layout, &text_width, &text_height);

  /* Position text */
  double x = 0;
  double y = (theme->height - text_height) / 2.0;

  /* Draw text shadow if enabled */
  if (style->shadow.enabled) {
    cairo_set_source_rgba(cr, style->shadow.color.r, style->shadow.color.g,
                          style->shadow.color.b, style->shadow.color.a);
    cairo_move_to(cr, x + style->shadow.offset_x, y + style->shadow.offset_y);
    pango_cairo_show_layout(cr, layout);
  }

  /* Draw text */
  cairo_set_color(cr, style->color);
  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, layout);

  g_object_unref(layout);
  cairo_destroy(cr);
  cairo_surface_flush(buffer->surface);

  if (tb->title) {
    wlr_scene_node_destroy(&tb->title->node);
  }
  tb->title = create_scene_buffer(tb->tree, buffer);

  /* Position title based on alignment */
  int title_x;
  switch (theme->title_align) {
  case ALIGN_LEFT:
    title_x = theme->buttons_left ? (theme->button_margin + btn_area + 10)
                                  : theme->padding_left;
    break;
  case ALIGN_RIGHT:
    title_x = tb->width - theme->padding_right - text_width;
    break;
  case ALIGN_CENTER:
  default:
    title_x = (tb->width - text_width) / 2;
    break;
  }

  wlr_scene_node_set_position(&tb->title->node, title_x, 0);

  wlr_buffer_drop(&buffer->base);
}

static void render_buttons(struct rendered_titlebar *tb,
                           struct titlebar_theme *theme, bool active) {
  int h = theme->height;

  /* Calculate button positions */
  int btn_y = (h - theme->btn_close.height) / 2;
  int x;

  struct button_theme *btns[3];
  enum button_state states[3];
  struct wlr_scene_buffer **scene_btns[3];

  struct box_pointers {
    int *x;
    int *y;
    int *w;
    int *h;
  };
  struct box_pointers boxes[3];

  if (theme->buttons_left) {
    /* macOS order: close, minimize, maximize */
    btns[0] = &theme->btn_close;
    btns[1] = &theme->btn_minimize;
    btns[2] = &theme->btn_maximize;
    states[0] = tb->close_state;
    states[1] = tb->min_state;
    states[2] = tb->max_state;
    scene_btns[0] = &tb->btn_close;
    scene_btns[1] = &tb->btn_minimize;
    scene_btns[2] = &tb->btn_maximize;

    boxes[0] =
        (struct box_pointers){&tb->close_box.x, &tb->close_box.y,
                              &tb->close_box.width, &tb->close_box.height};
    boxes[1] = (struct box_pointers){&tb->min_box.x, &tb->min_box.y,
                                     &tb->min_box.width, &tb->min_box.height};
    boxes[2] = (struct box_pointers){&tb->max_box.x, &tb->max_box.y,
                                     &tb->max_box.width, &tb->max_box.height};
    x = theme->button_margin;
  } else {
    /* Windows order: minimize, maximize, close (from right) */
    btns[0] = &theme->btn_minimize;
    btns[1] = &theme->btn_maximize;
    btns[2] = &theme->btn_close;
    states[0] = tb->min_state;
    states[1] = tb->max_state;
    states[2] = tb->close_state;
    scene_btns[0] = &tb->btn_minimize;
    scene_btns[1] = &tb->btn_maximize;
    scene_btns[2] = &tb->btn_close;

    boxes[0] = (struct box_pointers){&tb->min_box.x, &tb->min_box.y,
                                     &tb->min_box.width, &tb->min_box.height};
    boxes[1] = (struct box_pointers){&tb->max_box.x, &tb->max_box.y,
                                     &tb->max_box.width, &tb->max_box.height};
    boxes[2] =
        (struct box_pointers){&tb->close_box.x, &tb->close_box.y,
                              &tb->close_box.width, &tb->close_box.height};
    x = tb->width - theme->button_margin - btns[0]->width - btns[1]->width -
        btns[2]->width - theme->button_spacing * 2;
  }

  fprintf(stderr, "[TITLEBAR] Rendering %d buttons, starting at x=%d\n", 3, x);

  for (int i = 0; i < 3; i++) {
    struct button_theme *btn = btns[i];
    struct button_style *style;

    switch (states[i]) {
    case BTN_STATE_HOVER:
      style = &btn->hover;
      break;
    case BTN_STATE_PRESSED:
      style = &btn->pressed;
      break;
    default:
      style = &btn->normal;
      break;
    }

    fprintf(stderr, "[TITLEBAR] Button %d: type=%d, state=%d, pos=%d,%d, size=%dx%d\n", 
            i, btn->type, states[i], x, btn_y, btn->width, btn->height);

    struct cairo_buffer *buffer = cairo_buffer_create(btn->width, btn->height);
    if (!buffer)
      continue;

    cairo_t *cr = cairo_create(buffer->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    render_button(cr, 0, 0, btn, style, btn->type);

    cairo_destroy(cr);
    cairo_surface_flush(buffer->surface);

    if (*scene_btns[i]) {
      wlr_scene_node_destroy(&(*scene_btns[i])->node);
    }
    *scene_btns[i] = create_scene_buffer(tb->tree, buffer);
    wlr_scene_node_set_position(&(*scene_btns[i])->node, x, btn_y);

    /* Store hit box */
    *boxes[i].x = x;
    *boxes[i].y = btn_y;
    *boxes[i].w = btn->width;
    *boxes[i].h = btn->height;

    x += btn->width + theme->button_spacing;

    wlr_buffer_drop(&buffer->base);
  }
}

struct rendered_titlebar *titlebar_render_create(struct wlr_scene_tree *parent,
                                                 struct titlebar_theme *theme) {
  struct rendered_titlebar *tb = calloc(1, sizeof(*tb));
  if (!tb)
    return NULL;

  tb->tree = wlr_scene_tree_create(parent);
  if (!tb->tree) {
    free(tb);
    return NULL;
  }

  tb->width = 200;
  tb->height = theme->height;
  tb->active = true;

  return tb;
}

void titlebar_render_destroy(struct rendered_titlebar *tb) {
  if (!tb)
    return;

  if (tb->tree) {
    wlr_scene_node_destroy(&tb->tree->node);
  }
  free(tb);
}

void titlebar_render_update(struct rendered_titlebar *tb,
                            struct titlebar_theme *theme, int width,
                            const char *title, bool active) {
  if (!tb || !theme)
    return;

  bool needs_redraw = (tb->width != width) || (tb->active != active) ||
                      (title && strcmp(tb->cached_title, title) != 0);

  if (!needs_redraw)
    return;

  fprintf(stderr, "[TITLEBAR] Rendering titlebar: width=%d, title='%s', active=%d\n", 
          width, title ? title : "(null)", active);

  tb->width = width;
  tb->height = theme->height;
  tb->active = active;

  if (title) {
    strncpy(tb->cached_title, title, sizeof(tb->cached_title) - 1);
  }

  render_titlebar_background(tb, theme, active);
  render_title_text(tb, theme, title, active);
  render_buttons(tb, theme, active);
}

void titlebar_render_set_button_state(struct rendered_titlebar *tb,
                                      enum button_type btn,
                                      enum button_state state) {
  if (!tb)
    return;

  bool changed = false;

  switch (btn) {
  case BTN_TYPE_CLOSE:
    if (tb->close_state != state) {
      tb->close_state = state;
      changed = true;
    }
    break;
  case BTN_TYPE_MAXIMIZE:
    if (tb->max_state != state) {
      tb->max_state = state;
      changed = true;
    }
    break;
  case BTN_TYPE_MINIMIZE:
    if (tb->min_state != state) {
      tb->min_state = state;
      changed = true;
    }
    break;
  default:
    break;
  }

  if (changed && global_theme) {
    render_buttons(tb, global_theme, tb->active);
  }
}

enum button_type titlebar_render_hit_test(struct rendered_titlebar *tb, int x,
                                          int y) {
  if (!tb)
    return BTN_TYPE_CUSTOM;

  if (x >= tb->close_box.x && x < tb->close_box.x + tb->close_box.width &&
      y >= tb->close_box.y && y < tb->close_box.y + tb->close_box.height) {
    return BTN_TYPE_CLOSE;
  }

  if (x >= tb->max_box.x && x < tb->max_box.x + tb->max_box.width &&
      y >= tb->max_box.y && y < tb->max_box.y + tb->max_box.height) {
    return BTN_TYPE_MAXIMIZE;
  }

  if (x >= tb->min_box.x && x < tb->min_box.x + tb->min_box.width &&
      y >= tb->min_box.y && y < tb->min_box.y + tb->min_box.height) {
    return BTN_TYPE_MINIMIZE;
  }

  return BTN_TYPE_CUSTOM;
}

bool titlebar_render_in_drag_area(struct rendered_titlebar *tb, int x, int y) {
  if (!tb)
    return false;

  if (y >= 0 && y < tb->height && x >= 0 && x < tb->width) {
    return titlebar_render_hit_test(tb, x, y) == BTN_TYPE_CUSTOM;
  }

  return false;
}

void titlebar_set_global_theme(struct titlebar_theme *theme) {
  global_theme = theme;
}

struct titlebar_theme *titlebar_get_global_theme(void) { return global_theme; }

/* ============================================================================
 * Theme file loading
 * ============================================================================
 */

int titlebar_theme_load_from_config(struct titlebar_theme *theme,
                                    struct decor_config *config) {
  if (!theme || !config)
    return -1;

  fprintf(stderr, "[TITLEBAR] Loading theme from config\n");
  fprintf(stderr, "[TITLEBAR] Icon paths: close='%s', max='%s', min='%s'\n",
          config->icon_close_path, config->icon_maximize_path, config->icon_minimize_path);

  theme->height = config->height > 0 ? config->height : 34;
  theme->padding_left = 12;
  theme->padding_right = 12;
  theme->corner_radius_top = config->corner_radius >= 0 ? config->corner_radius : 10;

  theme->bg_color = color_from_hex(config->bg_color);
  theme->bg_color_inactive = color_from_hex(config->bg_color_inactive);

  theme->title.font_size = config->font_size > 0 ? config->font_size : 12;
  strncpy(theme->title.font_family, config->font, sizeof(theme->title.font_family) - 1);
  theme->title.font_weight = 600;
  theme->title.color = color_from_hex(config->title_color);
  theme->title_inactive.color = color_from_hex(config->title_color_inactive);
  theme->title_align = ALIGN_CENTER;

  theme->buttons_left = config->buttons_left;
  theme->button_spacing = config->button_spacing > 0 ? config->button_spacing : 8;
  theme->button_margin = 10;

 theme->btn_close.type = BTN_TYPE_CLOSE;
  theme->btn_close.shape = SHAPE_CIRCLE;
  theme->btn_close.width = config->button_size > 0 ? config->button_size : 16;
  theme->btn_close.height = config->button_size > 0 ? config->button_size : 16;
  theme->btn_close.normal.bg_color = color_from_hex(0xf38ba8ff);
  theme->btn_close.normal.icon_scale = 0.65f;  // ← CHANGE THIS FROM 0.0f
  theme->btn_close.normal.icon_color = color_from_hex(0x1e1e2eff); // Add a color
  theme->btn_close.hover.bg_color = color_from_hex(0xeba0acff);
  theme->btn_close.hover.icon_color = color_from_hex(0x1e1e2eff);
  theme->btn_close.hover.icon_scale = 0.65f;

  theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
  theme->btn_maximize.shape = SHAPE_CIRCLE;
  theme->btn_maximize.width = config->button_size > 0 ? config->button_size : 16;
  theme->btn_maximize.height = config->button_size > 0 ? config->button_size : 16;
  theme->btn_maximize.normal.bg_color = color_from_hex(0xa6e3a1ff);
  theme->btn_maximize.normal.icon_scale = 0.6f;  // ← CHANGE THIS FROM 0.0f
  theme->btn_maximize.normal.icon_color = color_from_hex(0x1e1e2eff);
  theme->btn_maximize.hover.bg_color = color_from_hex(0x94e2d5ff);
  theme->btn_maximize.hover.icon_color = color_from_hex(0x1e1e2eff);
  theme->btn_maximize.hover.icon_scale = 0.6f;

  theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
  theme->btn_minimize.shape = SHAPE_CIRCLE;
  theme->btn_minimize.width = config->button_size > 0 ? config->button_size : 16;
  theme->btn_minimize.height = config->button_size > 0 ? config->button_size : 16;
  theme->btn_minimize.normal.bg_color = color_from_hex(0xf9e2afff);
  theme->btn_minimize.normal.icon_scale = 0.6f;  // ← CHANGE THIS FROM 0.0f
  theme->btn_minimize.normal.icon_color = color_from_hex(0x1e1e2eff);
  theme->btn_minimize.hover.bg_color = color_from_hex(0xf5c2e7ff);
  theme->btn_minimize.hover.icon_color = color_from_hex(0x1e1e2eff);
  theme->btn_minimize.hover.icon_scale = 0.6f;

  theme->inactive_opacity = 0.85f;

  fprintf(stderr, "[TITLEBAR] Theme loaded successfully\n");
  return 0;
}
