/* visual_render.c - High-level rendering for titlebars and frames */
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "visual.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <drm_fourcc.h>
#include <pango/pangocairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================================
 * BUFFER IMPLEMENTATION
 * ============================================================================ */

struct visual_buffer {
    struct wlr_buffer base;
    cairo_surface_t *surface;
    void *data;
    int stride;
};

static void visual_buffer_destroy(struct wlr_buffer *wlr_buffer) {
    struct visual_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
    cairo_surface_destroy(buffer->surface);
    free(buffer->data);
    free(buffer);
}

static bool visual_buffer_begin_data_ptr_access(struct wlr_buffer *wlr_buffer,
        uint32_t flags, void **data, uint32_t *format, size_t *stride) {
    struct visual_buffer *buffer = wl_container_of(wlr_buffer, buffer, base);
    *data = buffer->data;
    *format = DRM_FORMAT_ARGB8888;
    *stride = buffer->stride;
    return true;
}

static void visual_buffer_end_data_ptr_access(struct wlr_buffer *wlr_buffer) {
    (void)wlr_buffer;
}

static const struct wlr_buffer_impl visual_buffer_impl = {
    .destroy = visual_buffer_destroy,
    .begin_data_ptr_access = visual_buffer_begin_data_ptr_access,
    .end_data_ptr_access = visual_buffer_end_data_ptr_access,
};

static struct visual_buffer *visual_buffer_create(int width, int height) {
    struct visual_buffer *buffer = calloc(1, sizeof(*buffer));
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
    
    wlr_buffer_init(&buffer->base, &visual_buffer_impl, width, height);
    return buffer;
}

/* ============================================================================
 * RENDER SURFACE
 * ============================================================================ */

render_surface_t *render_surface_create(struct wlr_scene_tree *parent, int width, int height) {
    render_surface_t *rs = calloc(1, sizeof(*rs));
    if (!rs) return NULL;
    
    rs->tree = wlr_scene_tree_create(parent);
    if (!rs->tree) {
        free(rs);
        return NULL;
    }
    
    struct visual_buffer *buffer = visual_buffer_create(width, height);
    if (!buffer) {
        wlr_scene_node_destroy(&rs->tree->node);
        free(rs);
        return NULL;
    }
    
    rs->buffer = wlr_scene_buffer_create(rs->tree, &buffer->base);
    rs->surface = buffer->surface;
    rs->cr = cairo_create(rs->surface);
    rs->width = width;
    rs->height = height;
    rs->dirty = true;
    
    wlr_buffer_drop(&buffer->base);
    
    return rs;
}

void render_surface_resize(render_surface_t *rs, int width, int height) {
    if (!rs || (rs->width == width && rs->height == height)) return;
    
    /* Destroy old buffer */
    if (rs->cr) cairo_destroy(rs->cr);
    if (rs->buffer) wlr_scene_node_destroy(&rs->buffer->node);
    
    /* Create new buffer */
    struct visual_buffer *buffer = visual_buffer_create(width, height);
    if (!buffer) return;
    
    rs->buffer = wlr_scene_buffer_create(rs->tree, &buffer->base);
    rs->surface = buffer->surface;
    rs->cr = cairo_create(rs->surface);
    rs->width = width;
    rs->height = height;
    rs->dirty = true;
    
    wlr_buffer_drop(&buffer->base);
}

void render_surface_commit(render_surface_t *rs) {
    if (!rs || !rs->dirty) return;
    
    cairo_surface_flush(rs->surface);
    
    /* Force scene buffer update by setting damage */
    struct wlr_fbox src_box = {
        .x = 0, .y = 0,
        .width = rs->width, .height = rs->height
    };
    wlr_scene_buffer_set_source_box(rs->buffer, &src_box);
    
    rs->dirty = false;
}

void render_surface_destroy(render_surface_t *rs) {
    if (!rs) return;
    
    if (rs->cr) cairo_destroy(rs->cr);
    if (rs->tree) wlr_scene_node_destroy(&rs->tree->node);
    free(rs);
}

/* ============================================================================
 * TITLEBAR RENDERING
 * ============================================================================ */

void draw_titlebar(render_surface_t *rs, titlebar_config_t *config,
                   const char *title, bool active, int hover_button) {
    if (!rs || !config || !rs->cr) return;
    
    cairo_t *cr = rs->cr;
    int w = rs->width;
    int h = config->height;
    
    /* Clear */
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    /* Draw shadow if configured */
    cairo_draw_shadow(cr, 0, 0, w, h,
        config->corner_radius_tl,
        &config->shadow);
    
    /* Draw background */
    rgba_t bg = active ? config->bg_color : config->bg_color_inactive;
    gradient_t *grad = active ? &config->bg_gradient : &config->bg_gradient_inactive;
    
    if (grad->type != GRAD_NONE) {
        cairo_pattern_t *pat = gradient_to_pattern(grad, 0, 0, w, h);
        if (pat) {
            cairo_set_source(cr, pat);
            cairo_pattern_destroy(pat);
        } else {
            cairo_set_rgba(cr, bg);
        }
    } else {
        cairo_set_rgba(cr, bg);
    }
    
    /* Path with per-corner radii */
    cairo_path_rounded_rect_corners(cr, 0, 0, w, h,
        config->corner_radius_tl, config->corner_radius_tr,
        config->corner_radius_br, config->corner_radius_bl);
    cairo_fill(cr);
    
    /* Draw inner shadow */
    cairo_draw_shadow(cr, 0, 0, w, h,
        config->corner_radius_tl, &config->inner_shadow);
    
    /* Draw border */
    border_t *border = active ? &config->border : &config->border_inactive;
    cairo_draw_border(cr, 0, 0, w, h, border);
    
    /* Draw separator if visible */
    if (config->separator_visible) {
        cairo_set_rgba(cr, config->separator_color);
        cairo_rectangle(cr, 0, h - config->separator_height,
                        w, config->separator_height);
        cairo_fill(cr);
    }
    
    /* Draw buttons */
    if (config->buttons_visible) {
        button_style_t *buttons[] = {
            &config->btn_close,
            &config->btn_maximize,
            &config->btn_minimize,
        };
        
        /* Calculate button positions */
        int total_btn_width = 0;
        int visible_count = 0;
        
        for (int i = 0; i < 4 && config->button_order[i] >= 0; i++) {
            int idx = config->button_order[i];
            if (idx >= 0 && idx < 3) {
                total_btn_width += buttons[idx]->width;
                visible_count++;
            }
        }
        total_btn_width += config->button_spacing * (visible_count - 1);
        
        int x;
        if (config->buttons_left) {
            x = config->button_margin;
        } else {
            x = w - config->button_margin - total_btn_width;
        }
        
        int y = (h - buttons[0]->height) / 2;
        
        for (int i = 0; i < 4 && config->button_order[i] >= 0; i++) {
            int idx = config->button_order[i];
            if (idx < 0 || idx >= 3) continue;
            
            button_style_t *btn = buttons[idx];
            button_state_t state = BTN_STATE_NORMAL;
            
            if (hover_button == idx) {
                state = BTN_STATE_HOVER;
            }
            
            draw_button(cr, x, y, btn, state);
            x += btn->width + config->button_spacing;
        }
    }
    
    /* Draw title */
    if (title && title[0]) {
        text_style_t *style = active ? &config->title_style : &config->title_style_inactive;
        
        /* Calculate available width for title */
        int btn_space = 0;
        if (config->buttons_visible) {
            int btn_count = 0;
            for (int i = 0; i < 4 && config->button_order[i] >= 0; i++) {
                btn_count++;
            }
            btn_space = config->button_margin * 2 +
                        config->btn_close.width * btn_count +
                        config->button_spacing * (btn_count - 1);
        }
        
        int text_max = w - config->padding_left - config->padding_right - btn_space - 20;
        if (config->title_max_width > 0 && text_max > config->title_max_width) {
            text_max = config->title_max_width;
        }
        if (text_max < 50) text_max = 50;
        
        /* Get text size for positioning */
        PangoLayout *layout = pango_cairo_create_layout(cr);
        char font_desc[256];
        snprintf(font_desc, sizeof(font_desc), "%s %d",
                 style->family, (int)style->size);
        PangoFontDescription *font = pango_font_description_from_string(font_desc);
        pango_layout_set_font_description(layout, font);
        pango_font_description_free(font);
        pango_layout_set_text(layout, title, -1);
        
        int text_width, text_height;
        pango_layout_get_pixel_size(layout, &text_width, &text_height);
        g_object_unref(layout);
        
        int text_x;
        switch (config->title_align) {
        case TEXT_ALIGN_LEFT:
            text_x = config->buttons_left ?
                     (config->button_margin + btn_space + 10) : config->padding_left;
            break;
        case TEXT_ALIGN_RIGHT:
            text_x = w - config->padding_right - text_width;
            break;
        case TEXT_ALIGN_CENTER:
        default:
            text_x = (w - text_width) / 2;
            break;
        }
        
        int text_y = (h - text_height) / 2 + config->title_offset_y;
        text_x += config->title_offset_x;
        
        draw_text(cr, text_x, text_y, text_max, title, style);
    }
    
    rs->dirty = true;
}

/* ============================================================================
 * WINDOW FRAME RENDERING
 * ============================================================================ */

void draw_window_frame(render_surface_t *rs, decoration_config_t *config,
                       int width, int height, bool active) {
    if (!rs || !config || !rs->cr) return;
    
    cairo_t *cr = rs->cr;
    
    /* Clear */
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    /* Draw window shadow */
    shadow_t *shadow = active ? &config->window_shadow : &config->window_shadow_inactive;
    border_t *border = active ? &config->window_border : &config->window_border_inactive;
    
    float radius = border->radius[0];
    
    cairo_draw_shadow(cr, 0, 0, width, height, radius, shadow);
    
    /* Draw border */
    cairo_draw_border(cr, 0, 0, width, height, border);
    
    rs->dirty = true;
}

/* ============================================================================
 * WORKSPACE INDICATOR RENDERING
 * ============================================================================ */

void draw_workspace_indicator(render_surface_t *rs, workspace_indicator_config_t *config,
                              int current, int total, int *urgent, int urgent_count) {
    if (!rs || !config || !config->enabled || !rs->cr) return;
    
    cairo_t *cr = rs->cr;
    
    /* Clear */
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    int w = rs->width;
    int h = rs->height;
    
    /* Draw background */
    if (config->bg_gradient.type != GRAD_NONE) {
        cairo_pattern_t *pat = gradient_to_pattern(&config->bg_gradient, 0, 0, w, h);
        if (pat) {
            cairo_set_source(cr, pat);
            cairo_pattern_destroy(pat);
        } else {
            cairo_set_rgba(cr, config->bg_color);
        }
    } else {
        cairo_set_rgba(cr, config->bg_color);
    }
    
    cairo_path_rounded_rect(cr, 0, 0, w, h, config->corner_radius);
    cairo_fill(cr);
    
    /* Draw shadow */
    cairo_draw_shadow(cr, 0, 0, w, h, config->corner_radius, &config->shadow);
    
    /* Draw border */
    cairo_draw_border(cr, 0, 0, w, h, &config->border);
    
    /* Calculate indicator positions */
    int total_width = total * config->indicator_size +
                      (total - 1) * config->indicator_spacing;
    int start_x = (w - total_width) / 2;
    int start_y = (h - config->indicator_size) / 2;
    
    /* Draw indicators */
    for (int i = 0; i < total; i++) {
        int x = start_x + i * (config->indicator_size + config->indicator_spacing);
        int y = start_y;
        
        /* Check if urgent */
        bool is_urgent = false;
        for (int j = 0; j < urgent_count; j++) {
            if (urgent[j] == i + 1) {
                is_urgent = true;
                break;
            }
        }
        
        rgba_t color;
        if (i + 1 == current) {
            color = config->active_color;
        } else if (is_urgent) {
            color = config->urgent_color;
        } else {
            color = config->inactive_color;
        }
        
        cairo_set_rgba(cr, color);
        
        switch (config->style) {
        case WS_STYLE_DOT:
            cairo_arc(cr, x + config->indicator_size / 2,
                     y + config->indicator_size / 2,
                     config->indicator_size / 2, 0, 2 * M_PI);
            cairo_fill(cr);
            break;
            
        case WS_STYLE_PILL:
            cairo_path_pill(cr, x, y,
                           (i + 1 == current) ? config->indicator_size * 2 : config->indicator_size,
                           config->indicator_size);
            cairo_fill(cr);
            break;
            
        case WS_STYLE_SQUARE:
            cairo_rectangle(cr, x, y, config->indicator_size, config->indicator_size);
            cairo_fill(cr);
            break;
            
        case WS_STYLE_NUMBER: {
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            draw_text(cr, x, y, config->indicator_size, num, &config->number_style);
            break;
        }
        }
    }
    
    rs->dirty = true;
}
