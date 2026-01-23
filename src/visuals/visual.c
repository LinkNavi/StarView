#define _POSIX_C_SOURCE 200809L

#include "visual.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pango/pangocairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================================
 * COLOR SYSTEM
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

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

rgba_t rgba_hsl(float h, float s, float l) {
    return rgba_hsla(h, s, l, 1.0f);
}

rgba_t rgba_hsla(float h, float s, float l, float a) {
    float r, g, b;
    
    if (s == 0) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hue_to_rgb(p, q, h + 1.0f/3);
        g = hue_to_rgb(p, q, h);
        b = hue_to_rgb(p, q, h - 1.0f/3);
    }
    
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

static void rgb_to_hsl(rgba_t c, float *h, float *s, float *l) {
    float max = fmaxf(c.r, fmaxf(c.g, c.b));
    float min = fminf(c.r, fminf(c.g, c.b));
    *l = (max + min) / 2;
    
    if (max == min) {
        *h = *s = 0;
    } else {
        float d = max - min;
        *s = *l > 0.5f ? d / (2 - max - min) : d / (max + min);
        
        if (max == c.r) {
            *h = (c.g - c.b) / d + (c.g < c.b ? 6 : 0);
        } else if (max == c.g) {
            *h = (c.b - c.r) / d + 2;
        } else {
            *h = (c.r - c.g) / d + 4;
        }
        *h /= 6;
    }
}

rgba_t rgba_saturate(rgba_t c, float amount) {
    float h, s, l;
    rgb_to_hsl(c, &h, &s, &l);
    s = fminf(1.0f, s + amount);
    rgba_t result = rgba_hsl(h, s, l);
    result.a = c.a;
    return result;
}

rgba_t rgba_desaturate(rgba_t c, float amount) {
    float h, s, l;
    rgb_to_hsl(c, &h, &s, &l);
    s = fmaxf(0.0f, s - amount);
    rgba_t result = rgba_hsl(h, s, l);
    result.a = c.a;
    return result;
}

rgba_t rgba_alpha(rgba_t c, float a) {
    return (rgba_t){ .r = c.r, .g = c.g, .b = c.b, .a = a };
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
 * GRADIENT SYSTEM
 * ============================================================================ */

gradient_t gradient_simple(gradient_type_t type, rgba_t start, rgba_t end) {
    gradient_t g = {0};
    g.type = type;
    g.stops[0] = (gradient_stop_t){ .position = 0.0f, .color = start };
    g.stops[1] = (gradient_stop_t){ .position = 1.0f, .color = end };
    g.stop_count = 2;
    g.cx = 0.5f;
    g.cy = 0.5f;
    g.radius = 1.0f;
    return g;
}

gradient_t gradient_three(gradient_type_t type, rgba_t a, rgba_t b, rgba_t c) {
    gradient_t g = {0};
    g.type = type;
    g.stops[0] = (gradient_stop_t){ .position = 0.0f, .color = a };
    g.stops[1] = (gradient_stop_t){ .position = 0.5f, .color = b };
    g.stops[2] = (gradient_stop_t){ .position = 1.0f, .color = c };
    g.stop_count = 3;
    g.cx = 0.5f;
    g.cy = 0.5f;
    g.radius = 1.0f;
    return g;
}

void gradient_add_stop(gradient_t *g, float pos, rgba_t color) {
    if (g->stop_count >= 8) return;
    g->stops[g->stop_count++] = (gradient_stop_t){ .position = pos, .color = color };
}

cairo_pattern_t *gradient_to_pattern(gradient_t *g, double x, double y, double w, double h) {
    if (g->type == GRAD_NONE || g->stop_count == 0) return NULL;
    
    cairo_pattern_t *pattern = NULL;
    
    switch (g->type) {
    case GRAD_LINEAR_V:
        pattern = cairo_pattern_create_linear(x, y, x, y + h);
        break;
    case GRAD_LINEAR_H:
        pattern = cairo_pattern_create_linear(x, y, x + w, y);
        break;
    case GRAD_LINEAR_DIAG:
        pattern = cairo_pattern_create_linear(x, y, x + w, y + h);
        break;
    case GRAD_LINEAR_DIAG2:
        pattern = cairo_pattern_create_linear(x + w, y, x, y + h);
        break;
    case GRAD_RADIAL: {
        double cx = x + w * g->cx;
        double cy = y + h * g->cy;
        double r = fmax(w, h) * g->radius;
        pattern = cairo_pattern_create_radial(cx, cy, 0, cx, cy, r);
        break;
    }
    case GRAD_RADIAL_CORNER:
        pattern = cairo_pattern_create_radial(x, y, 0, x, y, sqrt(w*w + h*h));
        break;
    default:
        return NULL;
    }
    
    for (int i = 0; i < g->stop_count; i++) {
        cairo_pattern_add_color_stop_rgba(pattern, g->stops[i].position,
            g->stops[i].color.r, g->stops[i].color.g,
            g->stops[i].color.b, g->stops[i].color.a);
    }
    
    return pattern;
}

/* ============================================================================
 * SHADOW SYSTEM
 * ============================================================================ */

shadow_t shadow_none(void) {
    return (shadow_t){ .enabled = false };
}

shadow_t shadow_drop(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0,
        .offset_y = blur * 0.5f,
        .blur = blur,
        .spread = 0,
        .color = color,
        .inset = false,
    };
}

shadow_t shadow_soft(float blur, float spread, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0,
        .offset_y = blur * 0.25f,
        .blur = blur,
        .spread = spread,
        .color = color,
        .inset = false,
    };
}

shadow_t shadow_hard(float ox, float oy, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = ox,
        .offset_y = oy,
        .blur = 0,
        .spread = 0,
        .color = color,
        .inset = false,
    };
}

shadow_t shadow_glow(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0,
        .offset_y = 0,
        .blur = blur,
        .spread = blur * 0.5f,
        .color = color,
        .inset = false,
    };
}

shadow_t shadow_inset(float blur, rgba_t color) {
    return (shadow_t){
        .enabled = true,
        .offset_x = 0,
        .offset_y = blur * 0.25f,
        .blur = blur,
        .spread = 0,
        .color = color,
        .inset = true,
    };
}

void cairo_draw_shadow(cairo_t *cr, double x, double y, double w, double h,
                       double radius, shadow_t *shadow) {
    if (!shadow || !shadow->enabled) return;
    
    cairo_save(cr);
    
    double ox = shadow->offset_x;
    double oy = shadow->offset_y;
    double blur = shadow->blur;
    double spread = shadow->spread;
    
    if (shadow->inset) {
        /* Inset shadow - draw inside the shape */
        cairo_path_rounded_rect(cr, x, y, w, h, radius);
        cairo_clip(cr);
        
        /* Draw shadow by stroking with decreasing alpha */
        int steps = (int)(blur * 2) + 1;
        for (int i = steps; i >= 0; i--) {
            float t = (float)i / steps;
            float alpha = shadow->color.a * (1 - t) * 0.5f;
            
            cairo_set_source_rgba(cr, shadow->color.r, shadow->color.g,
                                  shadow->color.b, alpha);
            cairo_set_line_width(cr, t * blur + 1);
            
            cairo_path_rounded_rect(cr, x + ox, y + oy, w, h, radius);
            cairo_stroke(cr);
        }
    } else {
        /* Drop shadow - draw behind the shape */
        int steps = (int)(blur / 2) + 1;
        if (steps < 1) steps = 1;
        
        for (int i = steps; i > 0; i--) {
            float t = (float)i / steps;
            float alpha = shadow->color.a * (1 - t * t) * 0.4f;
            float offset = t * blur;
            
            cairo_set_source_rgba(cr, shadow->color.r, shadow->color.g,
                                  shadow->color.b, alpha);
            
            cairo_path_rounded_rect(cr,
                x + ox - offset - spread,
                y + oy - offset - spread,
                w + offset * 2 + spread * 2,
                h + offset * 2 + spread * 2,
                radius + offset);
            cairo_fill(cr);
        }
    }
    
    cairo_restore(cr);
}

void cairo_draw_shadow_box(cairo_t *cr, double x, double y, double w, double h,
                           double *radii, shadow_t *shadow) {
    /* Simplified: use average radius */
    double avg = (radii[0] + radii[1] + radii[2] + radii[3]) / 4;
    cairo_draw_shadow(cr, x, y, w, h, avg, shadow);
}

/* ============================================================================
 * BORDER SYSTEM
 * ============================================================================ */

border_t border_none(void) {
    return (border_t){
        .width = 0,
        .style = BORDER_NONE,
    };
}

border_t border_solid(float width, rgba_t color, float radius) {
    return (border_t){
        .width = width,
        .color = color,
        .style = BORDER_SOLID,
        .radius = { radius, radius, radius, radius },
    };
}

border_t border_rounded(float width, rgba_t color, float tl, float tr, float br, float bl) {
    return (border_t){
        .width = width,
        .color = color,
        .style = BORDER_SOLID,
        .radius = { tl, tr, br, bl },
    };
}

void cairo_draw_border(cairo_t *cr, double x, double y, double w, double h, border_t *border) {
    if (!border || border->width <= 0 || border->style == BORDER_NONE) return;
    
    cairo_save(cr);
    cairo_set_rgba(cr, border->color);
    cairo_set_line_width(cr, border->width);
    
    switch (border->style) {
    case BORDER_DASHED:
        {
            double dashes[] = { border->width * 3, border->width * 2 };
            cairo_set_dash(cr, dashes, 2, 0);
        }
        break;
    case BORDER_DOTTED:
        {
            double dashes[] = { border->width, border->width * 2 };
            cairo_set_dash(cr, dashes, 2, 0);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        }
        break;
    default:
        break;
    }
    
    double hw = border->width / 2;
    cairo_path_rounded_rect_corners(cr,
        x + hw, y + hw, w - border->width, h - border->width,
        border->radius[0], border->radius[1], border->radius[2], border->radius[3]);
    cairo_stroke(cr);
    
    cairo_restore(cr);
}

/* ============================================================================
 * SHAPE DRAWING
 * ============================================================================ */

void cairo_path_rect(cairo_t *cr, double x, double y, double w, double h) {
    cairo_rectangle(cr, x, y, w, h);
}

void cairo_path_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
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

void cairo_path_rounded_rect_corners(cairo_t *cr, double x, double y, double w, double h,
                                     double tl, double tr, double br, double bl) {
    double max_r = fmin(w / 2, h / 2);
    tl = fmin(tl, max_r);
    tr = fmin(tr, max_r);
    br = fmin(br, max_r);
    bl = fmin(bl, max_r);
    
    cairo_new_sub_path(cr);
    
    if (tr > 0) {
        cairo_arc(cr, x + w - tr, y + tr, tr, -M_PI / 2, 0);
    } else {
        cairo_move_to(cr, x + w, y);
    }
    
    if (br > 0) {
        cairo_arc(cr, x + w - br, y + h - br, br, 0, M_PI / 2);
    } else {
        cairo_line_to(cr, x + w, y + h);
    }
    
    if (bl > 0) {
        cairo_arc(cr, x + bl, y + h - bl, bl, M_PI / 2, M_PI);
    } else {
        cairo_line_to(cr, x, y + h);
    }
    
    if (tl > 0) {
        cairo_arc(cr, x + tl, y + tl, tl, M_PI, 3 * M_PI / 2);
    } else {
        cairo_line_to(cr, x, y);
    }
    
    cairo_close_path(cr);
}

void cairo_path_circle(cairo_t *cr, double cx, double cy, double r) {
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
}

void cairo_path_ellipse(cairo_t *cr, double cx, double cy, double rx, double ry) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(cr);
}

void cairo_path_pill(cairo_t *cr, double x, double y, double w, double h) {
    double r = h / 2;
    if (w < h) r = w / 2;
    cairo_path_rounded_rect(cr, x, y, w, h, r);
}

/* Squircle using superellipse equation: |x|^n + |y|^n = 1 */
void cairo_path_squircle(cairo_t *cr, double x, double y, double w, double h, double n) {
    if (n <= 2) {
        cairo_path_rounded_rect(cr, x, y, w, h, fmin(w, h) * 0.25);
        return;
    }
    
    double cx = x + w / 2;
    double cy = y + h / 2;
    double rx = w / 2;
    double ry = h / 2;
    
    int segments = 64;
    cairo_new_sub_path(cr);
    
    for (int i = 0; i <= segments; i++) {
        double t = (double)i / segments * 2 * M_PI;
        double cos_t = cos(t);
        double sin_t = sin(t);
        
        double px = pow(fabs(cos_t), 2.0 / n) * (cos_t >= 0 ? 1 : -1) * rx;
        double py = pow(fabs(sin_t), 2.0 / n) * (sin_t >= 0 ? 1 : -1) * ry;
        
        if (i == 0) {
            cairo_move_to(cr, cx + px, cy + py);
        } else {
            cairo_line_to(cr, cx + px, cy + py);
        }
    }
    
    cairo_close_path(cr);
}

void cairo_path_diamond(cairo_t *cr, double x, double y, double w, double h) {
    cairo_new_sub_path(cr);
    cairo_move_to(cr, x + w / 2, y);
    cairo_line_to(cr, x + w, y + h / 2);
    cairo_line_to(cr, x + w / 2, y + h);
    cairo_line_to(cr, x, y + h / 2);
    cairo_close_path(cr);
}

void cairo_path_hexagon(cairo_t *cr, double cx, double cy, double r) {
    cairo_new_sub_path(cr);
    for (int i = 0; i < 6; i++) {
        double angle = M_PI / 3 * i - M_PI / 2;
        double px = cx + r * cos(angle);
        double py = cy + r * sin(angle);
        if (i == 0) {
            cairo_move_to(cr, px, py);
        } else {
            cairo_line_to(cr, px, py);
        }
    }
    cairo_close_path(cr);
}

void cairo_path_octagon(cairo_t *cr, double cx, double cy, double r) {
    cairo_new_sub_path(cr);
    for (int i = 0; i < 8; i++) {
        double angle = M_PI / 4 * i - M_PI / 8;
        double px = cx + r * cos(angle);
        double py = cy + r * sin(angle);
        if (i == 0) {
            cairo_move_to(cr, px, py);
        } else {
            cairo_line_to(cr, px, py);
        }
    }
    cairo_close_path(cr);
}
