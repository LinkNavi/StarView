/* visual_draw.c - High-level drawing functions */
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
 * ICON DRAWING
 * ============================================================================ */

icon_t icon_create(icon_type_t type, float size, rgba_t color) {
    return (icon_t){
        .type = type,
        .style = ICON_STYLE_LINE,
        .color = color,
        .secondary_color = rgba_alpha(color, 0.3f),
        .size = size,
        .stroke_width = size * 0.12f,
        .padding = 0,
        .antialiased = true,
    };
}

static void draw_icon_close(cairo_t *cr, double x, double y, double size, 
                            float stroke, icon_style_t style) {
    double d = size * 0.35;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    if (style == ICON_STYLE_FILLED) {
        /* Filled X with rounded ends */
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_width(cr, stroke * 1.5);
    } else {
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_width(cr, stroke);
    }
    
    cairo_move_to(cr, cx - d, cy - d);
    cairo_line_to(cr, cx + d, cy + d);
    cairo_stroke(cr);
    
    cairo_move_to(cr, cx + d, cy - d);
    cairo_line_to(cr, cx - d, cy + d);
    cairo_stroke(cr);
}

static void draw_icon_maximize(cairo_t *cr, double x, double y, double size,
                               float stroke, icon_style_t style) {
    double d = size * 0.30;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    cairo_set_line_width(cr, stroke);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    
    if (style == ICON_STYLE_FILLED) {
        cairo_rectangle(cr, cx - d, cy - d, d * 2, d * 2);
        cairo_fill(cr);
    } else {
        cairo_rectangle(cr, cx - d, cy - d, d * 2, d * 2);
        cairo_stroke(cr);
    }
}

static void draw_icon_restore(cairo_t *cr, double x, double y, double size,
                              float stroke, icon_style_t style) {
    double d = size * 0.25;
    double off = size * 0.1;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    cairo_set_line_width(cr, stroke);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    
    /* Back rectangle */
    cairo_rectangle(cr, cx - d + off, cy - d - off, d * 1.5, d * 1.5);
    cairo_stroke(cr);
    
    /* Front rectangle */
    if (style == ICON_STYLE_FILLED) {
        cairo_rectangle(cr, cx - d - off, cy - d + off, d * 1.5, d * 1.5);
        cairo_fill(cr);
    } else {
        cairo_rectangle(cr, cx - d - off, cy - d + off, d * 1.5, d * 1.5);
        cairo_stroke(cr);
    }
}

static void draw_icon_minimize(cairo_t *cr, double x, double y, double size,
                               float stroke, icon_style_t style) {
    double d = size * 0.30;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    (void)style;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, stroke * 1.2);
    
    cairo_move_to(cr, cx - d, cy);
    cairo_line_to(cr, cx + d, cy);
    cairo_stroke(cr);
}

static void draw_icon_plus(cairo_t *cr, double x, double y, double size,
                           float stroke, icon_style_t style) {
    double d = size * 0.30;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    (void)style;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, stroke);
    
    cairo_move_to(cr, cx - d, cy);
    cairo_line_to(cr, cx + d, cy);
    cairo_stroke(cr);
    
    cairo_move_to(cr, cx, cy - d);
    cairo_line_to(cr, cx, cy + d);
    cairo_stroke(cr);
}

static void draw_icon_check(cairo_t *cr, double x, double y, double size,
                            float stroke, icon_style_t style) {
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    (void)style;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, stroke * 1.2);
    
    cairo_move_to(cr, cx - size * 0.25, cy);
    cairo_line_to(cr, cx - size * 0.05, cy + size * 0.2);
    cairo_line_to(cr, cx + size * 0.25, cy - size * 0.2);
    cairo_stroke(cr);
}

static void draw_icon_chevron(cairo_t *cr, double x, double y, double size,
                              float stroke, int dir) {
    double d = size * 0.2;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, stroke);
    
    switch (dir) {
    case 0: /* up */
        cairo_move_to(cr, cx - d, cy + d * 0.5);
        cairo_line_to(cr, cx, cy - d * 0.5);
        cairo_line_to(cr, cx + d, cy + d * 0.5);
        break;
    case 1: /* down */
        cairo_move_to(cr, cx - d, cy - d * 0.5);
        cairo_line_to(cr, cx, cy + d * 0.5);
        cairo_line_to(cr, cx + d, cy - d * 0.5);
        break;
    case 2: /* left */
        cairo_move_to(cr, cx + d * 0.5, cy - d);
        cairo_line_to(cr, cx - d * 0.5, cy);
        cairo_line_to(cr, cx + d * 0.5, cy + d);
        break;
    case 3: /* right */
        cairo_move_to(cr, cx - d * 0.5, cy - d);
        cairo_line_to(cr, cx + d * 0.5, cy);
        cairo_line_to(cr, cx - d * 0.5, cy + d);
        break;
    }
    cairo_stroke(cr);
}

static void draw_icon_arrow(cairo_t *cr, double x, double y, double size,
                            float stroke, int dir) {
    double d = size * 0.25;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, stroke);
    
    switch (dir) {
    case 0: /* up */
        cairo_move_to(cr, cx, cy + d);
        cairo_line_to(cr, cx, cy - d);
        cairo_move_to(cr, cx - d * 0.7, cy - d * 0.3);
        cairo_line_to(cr, cx, cy - d);
        cairo_line_to(cr, cx + d * 0.7, cy - d * 0.3);
        break;
    case 1: /* down */
        cairo_move_to(cr, cx, cy - d);
        cairo_line_to(cr, cx, cy + d);
        cairo_move_to(cr, cx - d * 0.7, cy + d * 0.3);
        cairo_line_to(cr, cx, cy + d);
        cairo_line_to(cr, cx + d * 0.7, cy + d * 0.3);
        break;
    case 2: /* left */
        cairo_move_to(cr, cx + d, cy);
        cairo_line_to(cr, cx - d, cy);
        cairo_move_to(cr, cx - d * 0.3, cy - d * 0.7);
        cairo_line_to(cr, cx - d, cy);
        cairo_line_to(cr, cx - d * 0.3, cy + d * 0.7);
        break;
    case 3: /* right */
        cairo_move_to(cr, cx - d, cy);
        cairo_line_to(cr, cx + d, cy);
        cairo_move_to(cr, cx + d * 0.3, cy - d * 0.7);
        cairo_line_to(cr, cx + d, cy);
        cairo_line_to(cr, cx + d * 0.3, cy + d * 0.7);
        break;
    }
    cairo_stroke(cr);
}

static void draw_icon_hamburger(cairo_t *cr, double x, double y, double size,
                                float stroke, icon_style_t style) {
    double d = size * 0.25;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    (void)style;
    
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, stroke);
    
    cairo_move_to(cr, cx - d, cy - d * 0.8);
    cairo_line_to(cr, cx + d, cy - d * 0.8);
    cairo_stroke(cr);
    
    cairo_move_to(cr, cx - d, cy);
    cairo_line_to(cr, cx + d, cy);
    cairo_stroke(cr);
    
    cairo_move_to(cr, cx - d, cy + d * 0.8);
    cairo_line_to(cr, cx + d, cy + d * 0.8);
    cairo_stroke(cr);
}

static void draw_icon_dot(cairo_t *cr, double x, double y, double size,
                          icon_style_t style) {
    double cx = x + size / 2;
    double cy = y + size / 2;
    double r = size * 0.15;
    
    if (style == ICON_STYLE_FILLED) {
        cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
        cairo_fill(cr);
    } else {
        cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
}

static void draw_icon_ring(cairo_t *cr, double x, double y, double size,
                           float stroke) {
    double cx = x + size / 2;
    double cy = y + size / 2;
    double r = size * 0.3;
    
    cairo_set_line_width(cr, stroke);
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
    cairo_stroke(cr);
}

static void draw_icon_grid(cairo_t *cr, double x, double y, double size,
                           float stroke, icon_style_t style) {
    double s = size * 0.2;
    double gap = size * 0.1;
    double cx = x + size / 2;
    double cy = y + size / 2;
    
    (void)stroke;
    
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            double px = cx + i * (s + gap) - s / 2;
            double py = cy + j * (s + gap) - s / 2;
            
            if (style == ICON_STYLE_FILLED) {
                cairo_rectangle(cr, px, py, s, s);
                cairo_fill(cr);
            } else {
                cairo_rectangle(cr, px, py, s, s);
                cairo_stroke(cr);
            }
        }
    }
}

void cairo_draw_icon(cairo_t *cr, double x, double y, icon_t *icon) {
    if (!icon || icon->type == ICON_NONE) return;
    
    cairo_save(cr);
    
    if (icon->antialiased) {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    } else {
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    }
    
    cairo_set_rgba(cr, icon->color);
    
    double pad = icon->padding;
    double size = icon->size - pad * 2;
    double ix = x + pad;
    double iy = y + pad;
    
    switch (icon->type) {
    case ICON_CLOSE:
        draw_icon_close(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_MAXIMIZE:
        draw_icon_maximize(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_MINIMIZE:
        draw_icon_minimize(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_RESTORE:
        draw_icon_restore(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_PLUS:
        draw_icon_plus(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_MINUS:
        draw_icon_minimize(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_CHECK:
        draw_icon_check(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_X:
        draw_icon_close(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_CHEVRON_UP:
        draw_icon_chevron(cr, ix, iy, size, icon->stroke_width, 0);
        break;
    case ICON_CHEVRON_DOWN:
        draw_icon_chevron(cr, ix, iy, size, icon->stroke_width, 1);
        break;
    case ICON_CHEVRON_LEFT:
        draw_icon_chevron(cr, ix, iy, size, icon->stroke_width, 2);
        break;
    case ICON_CHEVRON_RIGHT:
        draw_icon_chevron(cr, ix, iy, size, icon->stroke_width, 3);
        break;
    case ICON_ARROW_UP:
        draw_icon_arrow(cr, ix, iy, size, icon->stroke_width, 0);
        break;
    case ICON_ARROW_DOWN:
        draw_icon_arrow(cr, ix, iy, size, icon->stroke_width, 1);
        break;
    case ICON_ARROW_LEFT:
        draw_icon_arrow(cr, ix, iy, size, icon->stroke_width, 2);
        break;
    case ICON_ARROW_RIGHT:
        draw_icon_arrow(cr, ix, iy, size, icon->stroke_width, 3);
        break;
    case ICON_HAMBURGER:
        draw_icon_hamburger(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    case ICON_DOT:
        draw_icon_dot(cr, ix, iy, size, icon->style);
        break;
    case ICON_RING:
        draw_icon_ring(cr, ix, iy, size, icon->stroke_width);
        break;
    case ICON_GRID:
        draw_icon_grid(cr, ix, iy, size, icon->stroke_width, icon->style);
        break;
    default:
        break;
    }
    
    cairo_restore(cr);
}

/* ============================================================================
 * TEXT STYLE
 * ============================================================================ */

text_style_t text_style_default(void) {
    return (text_style_t){
        .family = "sans-serif",
        .size = 12,
        .weight = FONT_WEIGHT_NORMAL,
        .italic = false,
        .color = rgba_hex(0x000000ff),
        .shadow = shadow_none(),
        .letter_spacing = 0,
        .line_height = 1.2f,
        .align = TEXT_ALIGN_LEFT,
        .overflow = TEXT_OVERFLOW_ELLIPSIS,
        .antialias = true,
        .subpixel = true,
    };
}

text_style_t text_style_create(const char *family, float size, font_weight_t weight, rgba_t color) {
    text_style_t style = text_style_default();
    strncpy(style.family, family, sizeof(style.family) - 1);
    style.size = size;
    style.weight = weight;
    style.color = color;
    return style;
}

void draw_text(cairo_t *cr, double x, double y, double max_width,
               const char *text, text_style_t *style) {
    if (!text || !text[0] || !style) return;
    
    cairo_save(cr);
    
    /* Setup Pango */
    PangoLayout *layout = pango_cairo_create_layout(cr);
    
    /* Build font description */
    char font_desc[256];
    const char *weight_str = "";
    switch (style->weight) {
    case FONT_WEIGHT_THIN: weight_str = "Thin "; break;
    case FONT_WEIGHT_EXTRA_LIGHT: weight_str = "ExtraLight "; break;
    case FONT_WEIGHT_LIGHT: weight_str = "Light "; break;
    case FONT_WEIGHT_MEDIUM: weight_str = "Medium "; break;
    case FONT_WEIGHT_SEMI_BOLD: weight_str = "SemiBold "; break;
    case FONT_WEIGHT_BOLD: weight_str = "Bold "; break;
    case FONT_WEIGHT_EXTRA_BOLD: weight_str = "ExtraBold "; break;
    case FONT_WEIGHT_BLACK: weight_str = "Black "; break;
    default: break;
    }
    
    snprintf(font_desc, sizeof(font_desc), "%s %s%s%.0f",
             style->family, weight_str,
             style->italic ? "Italic " : "",
             style->size);
    
    PangoFontDescription *font = pango_font_description_from_string(font_desc);
    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);
    
    pango_layout_set_text(layout, text, -1);
    
    if (max_width > 0) {
        pango_layout_set_width(layout, (int)(max_width * PANGO_SCALE));
        
        switch (style->overflow) {
        case TEXT_OVERFLOW_ELLIPSIS:
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
            break;
        case TEXT_OVERFLOW_CLIP:
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
            break;
        default:
            break;
        }
    }
    
    /* Letter spacing */
    if (style->letter_spacing != 0) {
        PangoAttrList *attrs = pango_attr_list_new();
        PangoAttribute *spacing = pango_attr_letter_spacing_new(
            (int)(style->letter_spacing * PANGO_SCALE));
        pango_attr_list_insert(attrs, spacing);
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
    }
    
    /* Get size for alignment */
    int text_width, text_height;
    pango_layout_get_pixel_size(layout, &text_width, &text_height);
    
    double tx = x;
    switch (style->align) {
    case TEXT_ALIGN_CENTER:
        if (max_width > 0) {
            tx = x + (max_width - text_width) / 2;
        }
        break;
    case TEXT_ALIGN_RIGHT:
        if (max_width > 0) {
            tx = x + max_width - text_width;
        }
        break;
    default:
        break;
    }
    
    /* Draw shadow if enabled */
    if (style->shadow.enabled) {
        cairo_set_source_rgba(cr,
            style->shadow.color.r, style->shadow.color.g,
            style->shadow.color.b, style->shadow.color.a);
        cairo_move_to(cr, tx + style->shadow.offset_x, y + style->shadow.offset_y);
        pango_cairo_show_layout(cr, layout);
    }
    
    /* Draw text */
    cairo_set_rgba(cr, style->color);
    cairo_move_to(cr, tx, y);
    pango_cairo_show_layout(cr, layout);
    
    g_object_unref(layout);
    cairo_restore(cr);
}

/* ============================================================================
 * BUTTON STYLES
 * ============================================================================ */

button_style_t button_style_circle(float size, rgba_t color, icon_type_t icon_type) {
    button_style_t style = {0};
    style.shape = SHAPE_CIRCLE;
    style.width = size;
    style.height = size;
    
    /* Normal state */
    style.states[BTN_STATE_NORMAL].bg_color = color;
    style.states[BTN_STATE_NORMAL].icon = icon_create(icon_type, size * 0.7f, rgba_hex(0x00000000));
    style.states[BTN_STATE_NORMAL].opacity = 1.0f;
    style.states[BTN_STATE_NORMAL].scale = 1.0f;
    
    /* Hover */
    style.states[BTN_STATE_HOVER].bg_color = rgba_lighten(color, 0.1f);
    style.states[BTN_STATE_HOVER].icon = icon_create(icon_type, size * 0.7f, rgba_darken(color, 0.4f));
    style.states[BTN_STATE_HOVER].opacity = 1.0f;
    style.states[BTN_STATE_HOVER].scale = 1.0f;
    
    /* Pressed */
    style.states[BTN_STATE_PRESSED].bg_color = rgba_darken(color, 0.1f);
    style.states[BTN_STATE_PRESSED].icon = icon_create(icon_type, size * 0.7f, rgba_darken(color, 0.5f));
    style.states[BTN_STATE_PRESSED].opacity = 1.0f;
    style.states[BTN_STATE_PRESSED].scale = 0.95f;
    
    /* Disabled */
    style.states[BTN_STATE_DISABLED].bg_color = rgba_desaturate(color, 0.5f);
    style.states[BTN_STATE_DISABLED].opacity = 0.5f;
    style.states[BTN_STATE_DISABLED].scale = 1.0f;
    
    style.transition_ms = 150;
    
    return style;
}

button_style_t button_style_rect(float w, float h, rgba_t color, icon_type_t icon_type) {
    button_style_t style = {0};
    style.shape = SHAPE_RECT;
    style.width = w;
    style.height = h;
    
    float icon_size = fminf(w, h) * 0.6f;
    
    /* Normal - transparent background */
    style.states[BTN_STATE_NORMAL].bg_color = rgba_hex(0x00000000);
    style.states[BTN_STATE_NORMAL].icon = icon_create(icon_type, icon_size, color);
    style.states[BTN_STATE_NORMAL].opacity = 1.0f;
    
    /* Hover */
    style.states[BTN_STATE_HOVER].bg_color = rgba_alpha(color, 0.1f);
    style.states[BTN_STATE_HOVER].icon = icon_create(icon_type, icon_size, color);
    style.states[BTN_STATE_HOVER].opacity = 1.0f;
    
    /* Pressed */
    style.states[BTN_STATE_PRESSED].bg_color = rgba_alpha(color, 0.2f);
    style.states[BTN_STATE_PRESSED].icon = icon_create(icon_type, icon_size, color);
    style.states[BTN_STATE_PRESSED].opacity = 1.0f;
    
    style.transition_ms = 100;
    
    return style;
}

button_style_t button_style_pill(float w, float h, rgba_t color, icon_type_t icon_type) {
    button_style_t style = button_style_rect(w, h, color, icon_type);
    style.shape = SHAPE_PILL;
    
    /* Add border for pill style */
    style.states[BTN_STATE_NORMAL].border = border_solid(1, rgba_alpha(color, 0.3f), h / 2);
    style.states[BTN_STATE_HOVER].border = border_solid(1, rgba_alpha(color, 0.5f), h / 2);
    style.states[BTN_STATE_PRESSED].border = border_solid(1, color, h / 2);
    
    return style;
}

void draw_button(cairo_t *cr, double x, double y, button_style_t *style,
                 button_state_t state) {
    if (!style) return;
    
    button_style_state_t *s = &style->states[state];
    
    cairo_save(cr);
    
    /* Apply scale transform */
    if (s->scale != 1.0f) {
        double cx = x + style->width / 2;
        double cy = y + style->height / 2;
        cairo_translate(cr, cx, cy);
        cairo_scale(cr, s->scale, s->scale);
        cairo_translate(cr, -cx, -cy);
    }
    
    /* Apply opacity */
    cairo_push_group(cr);
    
    /* Draw shadow */
    cairo_draw_shadow(cr, x, y, style->width, style->height,
                      style->shape == SHAPE_CIRCLE ? style->width / 2 : 0,
                      &s->shadow);
    
    /* Draw background */
    if (s->bg_gradient.type != GRAD_NONE) {
        cairo_pattern_t *pat = gradient_to_pattern(&s->bg_gradient, x, y,
                                                   style->width, style->height);
        if (pat) {
            cairo_set_source(cr, pat);
            cairo_pattern_destroy(pat);
        }
    } else {
        cairo_set_rgba(cr, s->bg_color);
    }
    
    switch (style->shape) {
    case SHAPE_CIRCLE:
        cairo_path_circle(cr, x + style->width / 2, y + style->height / 2,
                         fminf(style->width, style->height) / 2);
        break;
    case SHAPE_PILL:
        cairo_path_pill(cr, x, y, style->width, style->height);
        break;
    case SHAPE_SQUIRCLE:
        cairo_path_squircle(cr, x, y, style->width, style->height, style->squircle_n);
        break;
    case SHAPE_ROUNDED_RECT:
        cairo_path_rounded_rect(cr, x, y, style->width, style->height,
                               s->border.radius[0]);
        break;
    default:
        cairo_rectangle(cr, x, y, style->width, style->height);
        break;
    }
    cairo_fill(cr);
    
    /* Draw border */
    cairo_draw_border(cr, x, y, style->width, style->height, &s->border);
    
    /* Draw icon */
    if (s->icon.type != ICON_NONE) {
        double ix = x + (style->width - s->icon.size) / 2;
        double iy = y + (style->height - s->icon.size) / 2;
        cairo_draw_icon(cr, ix, iy, &s->icon);
    }
    
    /* Apply opacity */
    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, s->opacity);
    
    cairo_restore(cr);
}
