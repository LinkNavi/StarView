#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "titlebar_render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <drm_fourcc.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Global theme */
static struct titlebar_theme *global_theme = NULL;

/* ============================================================================
 * Buffer implementation for Cairo surfaces
 * ============================================================================ */

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
        uint32_t flags, void **data, uint32_t *format, size_t *stride) {
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

/* ============================================================================
 * Color helpers
 * ============================================================================ */

struct color color_from_hex(uint32_t hex) {
    return (struct color){
        .r = ((hex >> 24) & 0xff) / 255.0f,
        .g = ((hex >> 16) & 0xff) / 255.0f,
        .b = ((hex >> 8) & 0xff) / 255.0f,
        .a = (hex & 0xff) / 255.0f,
    };
}

struct color color_from_rgba(float r, float g, float b, float a) {
    return (struct color){ .r = r, .g = g, .b = b, .a = a };
}

uint32_t color_to_hex(struct color c) {
    return ((uint32_t)(c.r * 255) << 24) |
           ((uint32_t)(c.g * 255) << 16) |
           ((uint32_t)(c.b * 255) << 8) |
           (uint32_t)(c.a * 255);
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
 * ============================================================================ */

static void draw_rounded_rect(cairo_t *cr, double x, double y, 
                               double w, double h, double r) {
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

static void draw_rounded_rect_top(cairo_t *cr, double x, double y,
                                   double w, double h, double r) {
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

static void apply_gradient(cairo_t *cr, struct gradient *grad,
                           double x, double y, double w, double h) {
    if (grad->direction == GRADIENT_NONE) return;
    
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
    
    cairo_pattern_add_color_stop_rgba(pattern, 0, 
        grad->start.r, grad->start.g, grad->start.b, grad->start.a);
    cairo_pattern_add_color_stop_rgba(pattern, 1,
        grad->end.r, grad->end.g, grad->end.b, grad->end.a);
    
    cairo_set_source(cr, pattern);
    cairo_pattern_destroy(pattern);
}

static void draw_shadow(cairo_t *cr, double x, double y, double w, double h,
                        double radius, struct shadow_style *shadow) {
    if (!shadow->enabled) return;
    
    /* Simple shadow approximation using multiple offset rectangles */
    int steps = (int)(shadow->blur / 2) + 1;
    for (int i = steps; i > 0; i--) {
        float alpha = shadow->color.a * (1.0f - (float)i / (steps + 1)) * 0.3f;
        cairo_set_source_rgba(cr, shadow->color.r, shadow->color.g, 
                              shadow->color.b, alpha);
        
        double offset = (double)i * 0.5;
        draw_rounded_rect(cr, 
            x + shadow->offset_x - offset,
            y + shadow->offset_y - offset,
            w + offset * 2,
            h + offset * 2,
            radius + offset);
        cairo_fill(cr);
    }
}

/* ============================================================================
 * Button icon drawing
 * ============================================================================ */

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
 * ============================================================================ */

static void render_button(cairo_t *cr, double x, double y,
                          struct button_theme *theme,
                          struct button_style *style,
                          enum button_type type) {
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
    
    /* Draw icon */
    double icon_size = fmin(w, h) * style->icon_scale;
    
    switch (type) {
    case BTN_TYPE_CLOSE:
        draw_close_icon(cr, cx, cy, icon_size, style->icon_color);
        break;
    case BTN_TYPE_MAXIMIZE:
        draw_maximize_icon(cr, cx, cy, icon_size, style->icon_color);
        break;
    case BTN_TYPE_MINIMIZE:
        draw_minimize_icon(cr, cx, cy, icon_size, style->icon_color);
        break;
    default:
        break;
    }
}

/* ============================================================================
 * Theme presets
 * ============================================================================ */

struct titlebar_theme *titlebar_theme_create(void) {
    struct titlebar_theme *theme = calloc(1, sizeof(*theme));
    if (!theme) return NULL;
    
    /* Load default preset */
    titlebar_theme_load_preset(theme, THEME_PRESET_DEFAULT);
    
    return theme;
}

void titlebar_theme_destroy(struct titlebar_theme *theme) {
    if (!theme) return;
    free(theme->extra_buttons);
    free(theme);
}

void titlebar_theme_load_preset(struct titlebar_theme *theme, enum theme_preset preset) {
    memset(theme, 0, sizeof(*theme));
    
    switch (preset) {
    case THEME_PRESET_MACOS:
        /* macOS-style traffic lights */
        theme->height = 28;
        theme->padding_left = 8;
        theme->padding_right = 8;
        theme->corner_radius_top = 10;
        theme->bg_color = color_from_hex(0xe8e8e8ff);
        theme->bg_color_inactive = color_from_hex(0xf6f6f6ff);
        theme->bg_gradient.direction = GRADIENT_VERTICAL;
        theme->bg_gradient.start = color_from_hex(0xf8f8f8ff);
        theme->bg_gradient.end = color_from_hex(0xe0e0e0ff);
        
        theme->title.font_size = 13;
        strcpy(theme->title.font_family, "SF Pro Display, Helvetica Neue, sans-serif");
        theme->title.font_weight = 500;
        theme->title.color = color_from_hex(0x000000ff);
        theme->title_inactive.color = color_from_hex(0x00000080);
        theme->title_align = ALIGN_CENTER;
        
        theme->buttons_left = true;
        theme->button_spacing = 8;
        theme->button_margin = 8;
        
        /* Close button - red */
        theme->btn_close.type = BTN_TYPE_CLOSE;
        theme->btn_close.shape = SHAPE_CIRCLE;
        theme->btn_close.width = 12;
        theme->btn_close.height = 12;
        theme->btn_close.normal.bg_color = color_from_hex(0xff5f57ff);
        theme->btn_close.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_close.normal.icon_scale = 0.8f;
        theme->btn_close.hover.bg_color = color_from_hex(0xff5f57ff);
        theme->btn_close.hover.icon_color = color_from_hex(0x4d0000ff);
        theme->btn_close.hover.icon_scale = 0.8f;
        theme->btn_close.pressed.bg_color = color_from_hex(0xbf4942ff);
        theme->btn_close.pressed.icon_color = color_from_hex(0x4d0000ff);
        theme->btn_close.pressed.icon_scale = 0.8f;
        
        /* Minimize button - yellow */
        theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
        theme->btn_minimize.shape = SHAPE_CIRCLE;
        theme->btn_minimize.width = 12;
        theme->btn_minimize.height = 12;
        theme->btn_minimize.normal.bg_color = color_from_hex(0xfebc2eff);
        theme->btn_minimize.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_minimize.normal.icon_scale = 0.8f;
        theme->btn_minimize.hover.bg_color = color_from_hex(0xfebc2eff);
        theme->btn_minimize.hover.icon_color = color_from_hex(0x995700ff);
        theme->btn_minimize.hover.icon_scale = 0.8f;
        
        /* Maximize button - green */
        theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
        theme->btn_maximize.shape = SHAPE_CIRCLE;
        theme->btn_maximize.width = 12;
        theme->btn_maximize.height = 12;
        theme->btn_maximize.normal.bg_color = color_from_hex(0x28c840ff);
        theme->btn_maximize.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_maximize.normal.icon_scale = 0.8f;
        theme->btn_maximize.hover.bg_color = color_from_hex(0x28c840ff);
        theme->btn_maximize.hover.icon_color = color_from_hex(0x006500ff);
        theme->btn_maximize.hover.icon_scale = 0.8f;
        break;
        
    case THEME_PRESET_WINDOWS:
        /* Windows 11-style */
        theme->height = 32;
        theme->padding_left = 12;
        theme->padding_right = 0;
        theme->corner_radius_top = 8;
        theme->bg_color = color_from_hex(0xf3f3f3ff);
        theme->bg_color_inactive = color_from_hex(0xfbfbfbff);
        
        theme->title.font_size = 12;
        strcpy(theme->title.font_family, "Segoe UI, sans-serif");
        theme->title.font_weight = 400;
        theme->title.color = color_from_hex(0x000000ff);
        theme->title_align = ALIGN_LEFT;
        
        theme->buttons_left = false;
        theme->button_spacing = 0;
        theme->button_margin = 0;
        
        /* Windows-style rectangular buttons */
        theme->btn_close.shape = SHAPE_RECT;
        theme->btn_close.width = 46;
        theme->btn_close.height = 32;
        theme->btn_close.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_close.normal.icon_color = color_from_hex(0x000000ff);
        theme->btn_close.normal.icon_scale = 0.65f;
        theme->btn_close.hover.bg_color = color_from_hex(0xe81123ff);
        theme->btn_close.hover.icon_color = color_from_hex(0xffffffff);
        theme->btn_close.hover.icon_scale = 0.65f;
        
        theme->btn_maximize.shape = SHAPE_RECT;
        theme->btn_maximize.width = 46;
        theme->btn_maximize.height = 32;
        theme->btn_maximize.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_maximize.normal.icon_color = color_from_hex(0x000000ff);
        theme->btn_maximize.normal.icon_scale = 0.6f;
        theme->btn_maximize.hover.bg_color = color_from_hex(0x0000001a);
        theme->btn_maximize.hover.icon_color = color_from_hex(0x000000ff);
        theme->btn_maximize.hover.icon_scale = 0.6f;
        
        theme->btn_minimize.shape = SHAPE_RECT;
        theme->btn_minimize.width = 46;
        theme->btn_minimize.height = 32;
        theme->btn_minimize.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_minimize.normal.icon_color = color_from_hex(0x000000ff);
        theme->btn_minimize.normal.icon_scale = 0.6f;
        theme->btn_minimize.hover.bg_color = color_from_hex(0x0000001a);
        theme->btn_minimize.hover.icon_color = color_from_hex(0x000000ff);
        theme->btn_minimize.hover.icon_scale = 0.6f;
        break;
        
    case THEME_PRESET_GNOME:
        /* GNOME/Adwaita style */
        theme->height = 36;
        theme->padding_left = 10;
        theme->padding_right = 10;
        theme->corner_radius_top = 12;
        theme->bg_color = color_from_hex(0x303030ff);
        theme->bg_color_inactive = color_from_hex(0x404040ff);
        
        theme->title.font_size = 11;
        strcpy(theme->title.font_family, "Cantarell, sans-serif");
        theme->title.font_weight = 700;
        theme->title.color = color_from_hex(0xffffffff);
        theme->title_align = ALIGN_CENTER;
        
        theme->buttons_left = false;
        theme->button_spacing = 6;
        theme->button_margin = 6;
        
        theme->btn_close.shape = SHAPE_CIRCLE;
        theme->btn_close.width = 20;
        theme->btn_close.height = 20;
        theme->btn_close.normal.bg_color = color_from_hex(0x50505080);
        theme->btn_close.normal.icon_color = color_from_hex(0xffffffff);
        theme->btn_close.normal.icon_scale = 0.7f;
        theme->btn_close.hover.bg_color = color_from_hex(0xe0404080);
        theme->btn_close.hover.icon_color = color_from_hex(0xffffffff);
        theme->btn_close.hover.icon_scale = 0.7f;
        
        theme->btn_maximize.shape = SHAPE_CIRCLE;
        theme->btn_maximize.width = 20;
        theme->btn_maximize.height = 20;
        theme->btn_maximize.normal.bg_color = color_from_hex(0x50505080);
        theme->btn_maximize.normal.icon_color = color_from_hex(0xffffffff);
        theme->btn_maximize.normal.icon_scale = 0.65f;
        theme->btn_maximize.hover.bg_color = color_from_hex(0x60606080);
        theme->btn_maximize.hover.icon_color = color_from_hex(0xffffffff);
        theme->btn_maximize.hover.icon_scale = 0.65f;
        
        theme->btn_minimize.shape = SHAPE_CIRCLE;
        theme->btn_minimize.width = 20;
        theme->btn_minimize.height = 20;
        theme->btn_minimize.normal.bg_color = color_from_hex(0x50505080);
        theme->btn_minimize.normal.icon_color = color_from_hex(0xffffffff);
        theme->btn_minimize.normal.icon_scale = 0.65f;
        theme->btn_minimize.hover.bg_color = color_from_hex(0x60606080);
        theme->btn_minimize.hover.icon_color = color_from_hex(0xffffffff);
        theme->btn_minimize.hover.icon_scale = 0.65f;
        break;
        
    case THEME_PRESET_MINIMAL:
        /* Ultra minimal */
        theme->height = 24;
        theme->padding_left = 8;
        theme->padding_right = 8;
        theme->corner_radius_top = 0;
        theme->bg_color = color_from_hex(0x1e1e2eff);
        theme->bg_color_inactive = color_from_hex(0x313244ff);
        
        theme->title.font_size = 11;
        strcpy(theme->title.font_family, "monospace");
        theme->title.font_weight = 400;
        theme->title.color = color_from_hex(0xcdd6f4ff);
        theme->title_align = ALIGN_LEFT;
        
        theme->buttons_left = false;
        theme->button_spacing = 4;
        theme->button_margin = 4;
        
        theme->btn_close.shape = SHAPE_RECT;
        theme->btn_close.width = 16;
        theme->btn_close.height = 16;
        theme->btn_close.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_close.normal.icon_color = color_from_hex(0xf38ba8ff);
        theme->btn_close.normal.icon_scale = 0.6f;
        theme->btn_close.hover.bg_color = color_from_hex(0xf38ba820);
        theme->btn_close.hover.icon_color = color_from_hex(0xf38ba8ff);
        theme->btn_close.hover.icon_scale = 0.6f;
        
        theme->btn_maximize.shape = SHAPE_RECT;
        theme->btn_maximize.width = 16;
        theme->btn_maximize.height = 16;
        theme->btn_maximize.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_maximize.normal.icon_color = color_from_hex(0xa6e3a1ff);
        theme->btn_maximize.normal.icon_scale = 0.55f;
        theme->btn_maximize.hover.bg_color = color_from_hex(0xa6e3a120);
        theme->btn_maximize.hover.icon_color = color_from_hex(0xa6e3a1ff);
        theme->btn_maximize.hover.icon_scale = 0.55f;
        
        theme->btn_minimize.shape = SHAPE_RECT;
        theme->btn_minimize.width = 16;
        theme->btn_minimize.height = 16;
        theme->btn_minimize.normal.bg_color = color_from_hex(0x00000000);
        theme->btn_minimize.normal.icon_color = color_from_hex(0xf9e2afff);
        theme->btn_minimize.normal.icon_scale = 0.55f;
        theme->btn_minimize.hover.bg_color = color_from_hex(0xf9e2af20);
        theme->btn_minimize.hover.icon_color = color_from_hex(0xf9e2afff);
        theme->btn_minimize.hover.icon_scale = 0.55f;
        break;
        
    case THEME_PRESET_DEFAULT:
    default:
        /* Catppuccin-style default */
        theme->height = 30;
        theme->padding_left = 10;
        theme->padding_right = 10;
        theme->corner_radius_top = 8;
        theme->bg_color = color_from_hex(0x1e1e2eff);
        theme->bg_color_inactive = color_from_hex(0x313244ff);
        theme->bg_gradient.direction = GRADIENT_VERTICAL;
        theme->bg_gradient.start = color_from_hex(0x2a2a3cff);
        theme->bg_gradient.end = color_from_hex(0x1e1e2eff);
        
        theme->title.font_size = 12;
        strcpy(theme->title.font_family, "sans-serif");
        theme->title.font_weight = 600;
        theme->title.color = color_from_hex(0xcdd6f4ff);
        theme->title_inactive = theme->title;
        theme->title_inactive.color = color_from_hex(0x6c7086ff);
        theme->title_align = ALIGN_CENTER;
        
        theme->buttons_left = false;
        theme->button_spacing = 8;
        theme->button_margin = 8;
        
        theme->btn_close.type = BTN_TYPE_CLOSE;
        theme->btn_close.shape = SHAPE_CIRCLE;
        theme->btn_close.width = 14;
        theme->btn_close.height = 14;
        theme->btn_close.normal.bg_color = color_from_hex(0xf38ba8ff);
        theme->btn_close.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_close.normal.icon_scale = 0.7f;
        theme->btn_close.hover.bg_color = color_from_hex(0xeba0acff);
        theme->btn_close.hover.icon_color = color_from_hex(0x1e1e2eff);
        theme->btn_close.hover.icon_scale = 0.7f;
        theme->btn_close.pressed.bg_color = color_from_hex(0xd68096ff);
        theme->btn_close.pressed.icon_color = color_from_hex(0x1e1e2eff);
        theme->btn_close.pressed.icon_scale = 0.7f;
        
        theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
        theme->btn_maximize.shape = SHAPE_CIRCLE;
        theme->btn_maximize.width = 14;
        theme->btn_maximize.height = 14;
        theme->btn_maximize.normal.bg_color = color_from_hex(0xa6e3a1ff);
        theme->btn_maximize.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_maximize.normal.icon_scale = 0.65f;
        theme->btn_maximize.hover.bg_color = color_from_hex(0x94e2d5ff);
        theme->btn_maximize.hover.icon_color = color_from_hex(0x1e1e2eff);
        theme->btn_maximize.hover.icon_scale = 0.65f;
        
        theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
        theme->btn_minimize.shape = SHAPE_CIRCLE;
        theme->btn_minimize.width = 14;
        theme->btn_minimize.height = 14;
        theme->btn_minimize.normal.bg_color = color_from_hex(0xf9e2afff);
        theme->btn_minimize.normal.icon_color = color_from_hex(0x00000000);
        theme->btn_minimize.normal.icon_scale = 0.65f;
        theme->btn_minimize.hover.bg_color = color_from_hex(0xf5c2e7ff);
        theme->btn_minimize.hover.icon_color = color_from_hex(0x1e1e2eff);
        theme->btn_minimize.hover.icon_scale = 0.65f;
        break;
    }
    
    theme->inactive_opacity = 0.8f;
}

/* ============================================================================
 * Titlebar rendering
 * ============================================================================ */

static struct wlr_scene_buffer *create_scene_buffer(struct wlr_scene_tree *parent,
                                                     struct cairo_buffer *buffer) {
    return wlr_scene_buffer_create(parent, &buffer->base);
}

static void render_titlebar_background(struct rendered_titlebar *tb,
                                        struct titlebar_theme *theme,
                                        bool active) {
    int w = tb->width;
    int h = theme->height;
    
    struct cairo_buffer *buffer = cairo_buffer_create(w, h);
    if (!buffer) return;
    
    cairo_t *cr = cairo_create(buffer->surface);
    
    /* Clear */
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    /* Draw shadow if enabled */
    draw_shadow(cr, 0, 0, w, h, theme->corner_radius_top, &theme->shadow);
    
    /* Draw background */
    if (theme->bg_gradient.direction != GRADIENT_NONE && active) {
        apply_gradient(cr, &theme->bg_gradient, 0, 0, w, h);
    } else {
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

static void render_title_text(struct rendered_titlebar *tb,
                              struct titlebar_theme *theme,
                              const char *title,
                              bool active) {
    if (!title || !title[0]) return;
    
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
    
    int max_width = tb->width - theme->padding_left - theme->padding_right - btn_area - 20;
    if (max_width < 50) max_width = 50;
    
    /* Create buffer for text */
    struct cairo_buffer *buffer = cairo_buffer_create(max_width, theme->height);
    if (!buffer) return;
    
    cairo_t *cr = cairo_create(buffer->surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    /* Setup Pango */
    PangoLayout *layout = pango_cairo_create_layout(cr);
    
    char font_desc[128];
    snprintf(font_desc, sizeof(font_desc), "%s %s %d",
             style->font_family,
             style->font_weight >= 700 ? "Bold" : 
             style->font_weight >= 500 ? "Medium" : "",
             style->font_size);
    
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
        cairo_set_source_rgba(cr, 
            style->shadow.color.r, style->shadow.color.g,
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
        title_x = theme->buttons_left ? 
                  (theme->button_margin + btn_area + 10) : theme->padding_left;
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
                           struct titlebar_theme *theme,
                           bool active) {
    int h = theme->height;
    
    /* Calculate button positions */
    int btn_y = (h - theme->btn_close.height) / 2;
    int x;
    
    struct button_theme *btns[3];
    enum button_state states[3];
    struct wlr_scene_buffer **scene_btns[3];
    struct { int *x, *y, *w, *h; } boxes[3];
    
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
        boxes[0] = (typeof(boxes[0])){ &tb->close_box.x, &tb->close_box.y, 
                                        &tb->close_box.width, &tb->close_box.height };
        boxes[1] = (typeof(boxes[1])){ &tb->min_box.x, &tb->min_box.y,
                                        &tb->min_box.width, &tb->min_box.height };
        boxes[2] = (typeof(boxes[2])){ &tb->max_box.x, &tb->max_box.y,
                                        &tb->max_box.width, &tb->max_box.height };
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
        boxes[0] = (typeof(boxes[0])){ &tb->min_box.x, &tb->min_box.y,
                                        &tb->min_box.width, &tb->min_box.height };
        boxes[1] = (typeof(boxes[1])){ &tb->max_box.x, &tb->max_box.y,
                                        &tb->max_box.width, &tb->max_box.height };
        boxes[2] = (typeof(boxes[2])){ &tb->close_box.x, &tb->close_box.y,
                                        &tb->close_box.width, &tb->close_box.height };
        x = tb->width - theme->button_margin - 
            btns[0]->width - btns[1]->width - btns[2]->width -
            theme->button_spacing * 2;
    }
    
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
        
        struct cairo_buffer *buffer = cairo_buffer_create(btn->width, btn->height);
        if (!buffer) continue;
        
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
    if (!tb) return NULL;
    
    tb->tree = wlr_scene_tree_create(parent);
    if (!tb->tree) {
        free(tb);
        return NULL;
    }
    
    tb->width = 200;  /* Default, will be updated */
    tb->height = theme->height;
    tb->active = true;
    
    return tb;
}

void titlebar_render_destroy(struct rendered_titlebar *tb) {
    if (!tb) return;
    
    if (tb->tree) {
        wlr_scene_node_destroy(&tb->tree->node);
    }
    free(tb);
}

void titlebar_render_update(struct rendered_titlebar *tb,
                            struct titlebar_theme *theme,
                            int width,
                            const char *title,
                            bool active) {
    if (!tb || !theme) return;
    
    bool needs_redraw = (tb->width != width) || 
                        (tb->active != active) ||
                        (title && strcmp(tb->cached_title, title) != 0);
    
    if (!needs_redraw) return;
    
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
    if (!tb) return;
    
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

enum button_type titlebar_render_hit_test(struct rendered_titlebar *tb, int x, int y) {
    if (!tb) return BTN_TYPE_CUSTOM;
    
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
    
    return BTN_TYPE_CUSTOM;  /* None matched */
}

bool titlebar_render_in_drag_area(struct rendered_titlebar *tb, int x, int y) {
    if (!tb) return false;
    
    /* In titlebar but not on a button */
    if (y >= 0 && y < tb->height && x >= 0 && x < tb->width) {
        return titlebar_render_hit_test(tb, x, y) == BTN_TYPE_CUSTOM;
    }
    
    return false;
}

void titlebar_set_global_theme(struct titlebar_theme *theme) {
    global_theme = theme;
}

struct titlebar_theme *titlebar_get_global_theme(void) {
    return global_theme;
}

/* ============================================================================
 * Theme file loading (TOML format)
 * ============================================================================ */

int titlebar_theme_load_file(struct titlebar_theme *theme, const char *path) {
    /* TODO: Implement TOML parsing for custom themes */
    /* For now, just return error */
    (void)theme;
    (void)path;
    return -1;
}

int titlebar_theme_save_file(struct titlebar_theme *theme, const char *path) {
    /* TODO: Implement theme saving */
    (void)theme;
    (void)path;
    return -1;
}
