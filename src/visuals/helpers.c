/* helpers.c - Basic color and shape helper functions */
#define _POSIX_C_SOURCE 200809L

#include "../visual.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================================
 * COLOR HELPERS
 * ============================================================================ */

rgba_t rgba_hex(uint32_t hex) {
    return (rgba_t){
        .r = ((hex >> 24) & 0xff) / 255.0f,
        .g = ((hex >> 16) & 0xff) / 255.0f,
        .b = ((hex >> 8) & 0xff) / 255.0f,
        .a = (hex & 0xff) / 255.0f,
    };
}

rgba_t rgba_rgb(float r, float g, float b) {
    return (rgba_t){ .r = r, .g = g, .b = b, .a = 1.0f };
}

rgba_t rgba_rgba(float r, float g, float b, float a) {
    return (rgba_t){ .r = r, .g = g, .b = b, .a = a };
}

rgba_t rgba_blend(rgba_t a, rgba_t b, float t) {
    return (rgba_t){
        .r = a.r + (b.r - a.r) * t,
        .g = a.g + (b.g - a.g) * t,
        .b = a.b + (b.b - a.b) * t,
        .a = a.a + (b.a - a.a) * t,
    };
}

rgba_t rgba_lighten(rgba_t c, float amount) {
    return (rgba_t){
        .r = fminf(1.0f, c.r + amount),
        .g = fminf(1.0f, c.g + amount),
        .b = fminf(1.0f, c.b + amount),
        .a = c.a,
    };
}

rgba_t rgba_darken(rgba_t c, float amount) {
    return (rgba_t){
        .r = fmaxf(0.0f, c.r - amount),
        .g = fmaxf(0.0f, c.g - amount),
        .b = fmaxf(0.0f, c.b - amount),
        .a = c.a,
    };
}

rgba_t rgba_saturate(rgba_t c, float amount) {
    float gray = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
    return (rgba_t){
        .r = c.r + (c.r - gray) * amount,
        .g = c.g + (c.g - gray) * amount,
        .b = c.b + (c.b - gray) * amount,
        .a = c.a,
    };
}

rgba_t rgba_desaturate(rgba_t c, float amount) {
    return rgba_saturate(c, -amount);
}

rgba_t rgba_alpha(rgba_t c, float a) {
    c.a = a;
    return c;
}

uint32_t rgba_to_hex(rgba_t c) {
    return ((uint32_t)(c.r * 255) << 24) |
           ((uint32_t)(c.g * 255) << 16) |
           ((uint32_t)(c.b * 255) << 8) |
           (uint32_t)(c.a * 255);
}

void cairo_set_rgba(cairo_t *cr, rgba_t c) {
    cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a);
}

/* ============================================================================
 * GRADIENT HELPERS
 * ============================================================================ */

gradient_t gradient_simple(gradient_type_t type, rgba_t start, rgba_t end) {
    gradient_t g = {0};
    g.type = type;
    g.stops[0] = (gradient_stop_t){ .position = 0.0f, .color = start };
    g.stops[1] = (gradient_stop_t){ .position = 1.0f, .color = end };
    g.stop_count = 2;
    return g;
}

void gradient_add_stop(gradient_t *g, float pos, rgba_t color) {
    if (!g || g->stop_count >= 8) return;
    g->stops[g->stop_count++] = (gradient_stop_t){ .position = pos, .color = color };
}

cairo_pattern_t *gradient_to_pattern(gradient_t *g, double x, double y, double w, double h) {
    if (!g || g->type == GRAD_NONE) return NULL;
    
    cairo_pattern_t *pat = NULL;
    
    switch (g->type) {
    case GRAD_LINEAR_V:
        pat = cairo_pattern_create_linear(x, y, x, y + h);
        break;
    case GRAD_LINEAR_H:
        pat = cairo_pattern_create_linear(x, y, x + w, y);
        break;
    case GRAD_LINEAR_DIAG:
        pat = cairo_pattern_create_linear(x, y, x + w, y + h);
        break;
    case GRAD_RADIAL:
        pat = cairo_pattern_create_radial(x + w/2, y + h/2, 0, x + w/2, y + h/2, fmax(w, h)/2);
        break;
    default:
        return NULL;
    }
    
    for (int i = 0; i < g->stop_count; i++) {
        rgba_t c = g->stops[i].color;
        cairo_pattern_add_color_stop_rgba(pat, g->stops[i].position, c.r, c.g, c.b, c.a);
    }
    
    return pat;
}

/* ============================================================================
 * SHADOW HELPERS
 * ============================================================================ */

shadow_t shadow_none(void) {
    return (shadow_t){ .enabled = false };
}

shadow_t shadow_drop(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0, .offset_y = 4,
        .blur = blur, .spread = 0,
        .color = color,
        .inset = false
    };
}

shadow_t shadow_soft(float blur, float spread, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0, .offset_y = 2,
        .blur = blur, .spread = spread,
        .color = color,
        .inset = false
    };
}

shadow_t shadow_hard(float ox, float oy, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = ox, .offset_y = oy,
        .blur = 0, .spread = 0,
        .color = color,
        .inset = false
    };
}

shadow_t shadow_glow(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0, .offset_y = 0,
        .blur = blur, .spread = 0,
        .color = color,
        .inset = false
    };
}

shadow_t shadow_inset(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0, .offset_y = 0,
        .blur = blur, .spread = 0,
        .color = color,
        .inset = true
    };
}

void cairo_draw_shadow(cairo_t *cr, double x, double y, double w, double h,
                       double radius, shadow_t *shadow) {
    if (!shadow || !shadow->enabled) return;
    
    int steps = (int)(shadow->blur / 2) + 1;
    for (int i = steps; i > 0; i--) {
        float alpha = shadow->color.a * (1.0f - (float)i / (steps + 1)) * 0.3f;
        cairo_set_source_rgba(cr, shadow->color.r, shadow->color.g, 
                              shadow->color.b, alpha);
        
        double offset = (double)i * 0.5;
        cairo_path_rounded_rect(cr, 
            x + shadow->offset_x - offset,
            y + shadow->offset_y - offset,
            w + offset * 2,
            h + offset * 2,
            radius + offset);
        cairo_fill(cr);
    }
}

/* ============================================================================
 * BORDER HELPERS
 * ============================================================================ */

border_t border_none(void) {
    return (border_t){ .width = 0, .style = BORDER_NONE };
}

border_t border_solid(float width, rgba_t color, float radius) {
    return (border_t){
        .width = width,
        .color = color,
        .style = BORDER_SOLID,
        .radius = { radius, radius, radius, radius }
    };
}

border_t border_rounded(float width, rgba_t color, float tl, float tr, float br, float bl) {
    return (border_t){
        .width = width,
        .color = color,
        .style = BORDER_SOLID,
        .radius = { tl, tr, br, bl }
    };
}

void cairo_draw_border(cairo_t *cr, double x, double y, double w, double h, border_t *border) {
    if (!border || border->width <= 0 || border->style == BORDER_NONE) return;
    
    cairo_set_rgba(cr, border->color);
    cairo_set_line_width(cr, border->width);
    
    if (border->radius[0] > 0 || border->radius[1] > 0 || 
        border->radius[2] > 0 || border->radius[3] > 0) {
        cairo_path_rounded_rect_corners(cr, x, y, w, h,
            border->radius[0], border->radius[1], border->radius[2], border->radius[3]);
    } else {
        cairo_rectangle(cr, x, y, w, h);
    }
    
    cairo_stroke(cr);
}

/* ============================================================================
 * SHAPE PATHS
 * ============================================================================ */

void cairo_path_rect(cairo_t *cr, double x, double y, double w, double h) {
    cairo_rectangle(cr, x, y, w, h);
}

void cairo_path_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_path_rounded_rect_corners(cr, x, y, w, h, r, r, r, r);
}

void cairo_path_rounded_rect_corners(cairo_t *cr, double x, double y, double w, double h,
                                     double tl, double tr, double br, double bl) {
    cairo_new_sub_path(cr);
    
    if (tr > 0) {
        cairo_arc(cr, x + w - tr, y + tr, tr, -M_PI/2, 0);
    } else {
        cairo_move_to(cr, x + w, y);
    }
    
    if (br > 0) {
        cairo_arc(cr, x + w - br, y + h - br, br, 0, M_PI/2);
    } else {
        cairo_line_to(cr, x + w, y + h);
    }
    
    if (bl > 0) {
        cairo_arc(cr, x + bl, y + h - bl, bl, M_PI/2, M_PI);
    } else {
        cairo_line_to(cr, x, y + h);
    }
    
    if (tl > 0) {
        cairo_arc(cr, x + tl, y + tl, tl, M_PI, 3*M_PI/2);
    } else {
        cairo_line_to(cr, x, y);
    }
    
    cairo_close_path(cr);
}

void cairo_path_circle(cairo_t *cr, double cx, double cy, double r) {
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
}

void cairo_path_pill(cairo_t *cr, double x, double y, double w, double h) {
    double r = h / 2;
    cairo_path_rounded_rect(cr, x, y, w, h, r);
}

void cairo_path_squircle(cairo_t *cr, double x, double y, double w, double h, double n) {
    if (n == 0) n = 4.0; // Default superellipse parameter
    
    double cx = x + w / 2;
    double cy = y + h / 2;
    double a = w / 2;
    double b = h / 2;
    
    cairo_new_path(cr);
    
    int steps = 100;
    for (int i = 0; i <= steps; i++) {
        double t = 2.0 * M_PI * i / steps;
        double cost = cos(t);
        double sint = sin(t);
        
        double px = cx + a * pow(fabs(cost), 2.0/n) * (cost >= 0 ? 1 : -1);
        double py = cy + b * pow(fabs(sint), 2.0/n) * (sint >= 0 ? 1 : -1);
        
        if (i == 0) {
            cairo_move_to(cr, px, py);
        } else {
            cairo_line_to(cr, px, py);
        }
    }
    
    cairo_close_path(cr);
}

/* ============================================================================
 * THEME CONFIG HELPERS
 * ============================================================================ */

titlebar_config_t titlebar_config_default(void) {
    titlebar_config_t cfg = {0};
    
    cfg.height = 30;
    cfg.padding_left = 10;
    cfg.padding_right = 10;
    cfg.bg_color = rgba_hex(0x1e1e2eff);
    cfg.bg_color_inactive = rgba_hex(0x313244ff);
    cfg.bg_gradient.type = GRAD_NONE;
    
    cfg.corner_radius_tl = 8;
    cfg.corner_radius_tr = 8;
    
    cfg.title_style = text_style_create("sans-serif", 12, FONT_WEIGHT_NORMAL, rgba_hex(0xcdd6f4ff));
    cfg.title_style_inactive = text_style_create("sans-serif", 12, FONT_WEIGHT_NORMAL, rgba_hex(0x6c7086ff));
    cfg.title_align = TEXT_ALIGN_CENTER;
    
    cfg.buttons_visible = true;
    cfg.buttons_left = false;
    cfg.button_spacing = 8;
    cfg.button_margin = 8;
    
    cfg.btn_close = button_style_circle(14, rgba_hex(0xf38ba8ff), ICON_CLOSE);
    cfg.btn_maximize = button_style_circle(14, rgba_hex(0xa6e3a1ff), ICON_MAXIMIZE);
    cfg.btn_minimize = button_style_circle(14, rgba_hex(0xf9e2afff), ICON_MINIMIZE);
    
    cfg.button_order[0] = 2;
    cfg.button_order[1] = 1;
    cfg.button_order[2] = 0;
    cfg.button_order[3] = -1;
    
    return cfg;
}

titlebar_config_t titlebar_config_preset(theme_preset_t preset) {
    (void)preset;
    return titlebar_config_default();
}
