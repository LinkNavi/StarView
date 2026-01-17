#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

// Color conversion helper
static void color_to_float(uint32_t color, float out[4]) {
    out[0] = ((color >> 24) & 0xff) / 255.0f;
    out[1] = ((color >> 16) & 0xff) / 255.0f;
    out[2] = ((color >> 8) & 0xff) / 255.0f;
    out[3] = (color & 0xff) / 255.0f;
}

// Create a titlebar element (button, indicator, etc.)
struct titlebar_element *titlebar_element_create(
    struct titlebar *titlebar, 
    enum titlebar_element_type type,
    int x, int y, int width, int height,
    uint32_t color) {
    
    if (!titlebar || !titlebar->tree) return NULL;
    
    struct titlebar_element *elem = calloc(1, sizeof(*elem));
    if (!elem) return NULL;
    
    elem->type = type;
    elem->x = x;
    elem->y = y;
    elem->width = width;
    elem->height = height;
    elem->color = color;
    elem->visible = true;
    elem->enabled = true;
    
    float rgba[4];
    color_to_float(color, rgba);
    
    elem->rect = wlr_scene_rect_create(titlebar->tree, width, height, rgba);
    if (!elem->rect) {
        free(elem);
        return NULL;
    }
    
    wlr_scene_node_set_position(&elem->rect->node, x, y);
    
    // Add to titlebar's element list
    if (titlebar->element_count >= 32) {
        wlr_scene_node_destroy(&elem->rect->node);
        free(elem);
        return NULL;
    }
    
    titlebar->elements[titlebar->element_count++] = elem;
    
    return elem;
}

// Create a standard button
struct titlebar_element *titlebar_button_create(
    struct titlebar *titlebar,
    int x, int y,
    uint32_t color,
    void (*on_click)(struct toplevel *)) {
    
    int size = config.decor.button_size;
    struct titlebar_element *btn = titlebar_element_create(
        titlebar, TITLEBAR_BUTTON, x, y, size, size, color);
    
    if (btn) {
        btn->on_click = on_click;
    }
    
    return btn;
}

// Create a text label (placeholder rect for now)
struct titlebar_element *titlebar_label_create(
    struct titlebar *titlebar,
    int x, int y,
    int width,
    const char *text,
    uint32_t color) {
    
    int height = config.decor.height;
    struct titlebar_element *label = titlebar_element_create(
        titlebar, TITLEBAR_LABEL, x, y, width, height, color);
    
    if (label && text) {
        strncpy(label->text, text, sizeof(label->text) - 1);
    }
    
    return label;
}

// Create an indicator (small colored dot/rect)
struct titlebar_element *titlebar_indicator_create(
    struct titlebar *titlebar,
    int x, int y,
    int size,
    uint32_t color) {
    
    return titlebar_element_create(
        titlebar, TITLEBAR_INDICATOR, x, y, size, size, color);
}

// Create a spacer (invisible element for layout)
struct titlebar_element *titlebar_spacer_create(
    struct titlebar *titlebar,
    int x, int y,
    int width) {
    
    struct titlebar_element *spacer = titlebar_element_create(
        titlebar, TITLEBAR_SPACER, x, y, width, config.decor.height, 0);
    
    if (spacer && spacer->rect) {
        wlr_scene_node_set_enabled(&spacer->rect->node, false);
    }
    
    return spacer;
}

// Update element position
void titlebar_element_set_position(struct titlebar_element *elem, int x, int y) {
    if (!elem || !elem->rect) return;
    
    elem->x = x;
    elem->y = y;
    wlr_scene_node_set_position(&elem->rect->node, x, y);
}

// Update element size
void titlebar_element_set_size(struct titlebar_element *elem, int width, int height) {
    if (!elem || !elem->rect) return;
    
    elem->width = width;
    elem->height = height;
    wlr_scene_rect_set_size(elem->rect, width, height);
}

// Update element color
void titlebar_element_set_color(struct titlebar_element *elem, uint32_t color) {
    if (!elem || !elem->rect) return;
    
    elem->color = color;
    float rgba[4];
    color_to_float(color, rgba);
    wlr_scene_rect_set_color(elem->rect, rgba);
}

// Set element visibility
void titlebar_element_set_visible(struct titlebar_element *elem, bool visible) {
    if (!elem || !elem->rect) return;
    
    elem->visible = visible;
    wlr_scene_node_set_enabled(&elem->rect->node, visible);
}

// Set element text (for labels)
void titlebar_element_set_text(struct titlebar_element *elem, const char *text) {
    if (!elem || elem->type != TITLEBAR_LABEL) return;
    
    if (text) {
        strncpy(elem->text, text, sizeof(elem->text) - 1);
        elem->text[sizeof(elem->text) - 1] = '\0';
    }
}

// Hit test an element
bool titlebar_element_contains(struct titlebar_element *elem, int x, int y) {
    if (!elem || !elem->visible || !elem->enabled) return false;
    
    return (x >= elem->x && x < elem->x + elem->width &&
            y >= elem->y && y < elem->y + elem->height);
}

// Auto-layout helper: arrange elements horizontally
void titlebar_layout_horizontal(
    struct titlebar *titlebar,
    struct titlebar_element **elements,
    int count,
    int start_x,
    int spacing) {
    
    int x = start_x;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        
        titlebar_element_set_position(elements[i], x, 
            (config.decor.height - elements[i]->height) / 2);
        x += elements[i]->width + spacing;
    }
}

// Auto-layout helper: arrange elements vertically
void titlebar_layout_vertical(
    struct titlebar *titlebar,
    struct titlebar_element **elements,
    int count,
    int start_y,
    int spacing) {
    
    int y = start_y;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        
        titlebar_element_set_position(elements[i], 
            (titlebar->width - elements[i]->width) / 2, y);
        y += elements[i]->height + spacing;
    }
}

// Layout helper: right-align elements
void titlebar_layout_right_align(
    struct titlebar *titlebar,
    struct titlebar_element **elements,
    int count,
    int spacing) {
    
    int total_width = 0;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        total_width += elements[i]->width;
    }
    total_width += spacing * (count - 1);
    
    int x = titlebar->width - total_width - spacing;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        
        titlebar_element_set_position(elements[i], x,
            (config.decor.height - elements[i]->height) / 2);
        x += elements[i]->width + spacing;
    }
}

// Layout helper: center elements
void titlebar_layout_center(
    struct titlebar *titlebar,
    struct titlebar_element **elements,
    int count,
    int spacing) {
    
    int total_width = 0;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        total_width += elements[i]->width;
    }
    total_width += spacing * (count - 1);
    
    int x = (titlebar->width - total_width) / 2;
    for (int i = 0; i < count; i++) {
        if (!elements[i]) continue;
        
        titlebar_element_set_position(elements[i], x,
            (config.decor.height - elements[i]->height) / 2);
        x += elements[i]->width + spacing;
    }
}

// Find element at position
struct titlebar_element *titlebar_element_at(
    struct titlebar *titlebar,
    int x, int y) {
    
    if (!titlebar) return NULL;
    
    for (int i = 0; i < titlebar->element_count; i++) {
        struct titlebar_element *elem = titlebar->elements[i];
        if (titlebar_element_contains(elem, x, y)) {
            return elem;
        }
    }
    
    return NULL;
}

// Handle element click
void titlebar_element_click(struct titlebar_element *elem, struct toplevel *toplevel) {
    if (!elem || !elem->enabled) return;
    
    if (elem->on_click) {
        elem->on_click(toplevel);
    }
}

// Preset layouts
void titlebar_apply_preset_windows(struct titlebar *titlebar) {
    if (!titlebar) return;
    
    // Clear existing elements
    for (int i = 0; i < titlebar->element_count; i++) {
        if (titlebar->elements[i] && titlebar->elements[i]->rect) {
            wlr_scene_node_destroy(&titlebar->elements[i]->rect->node);
        }
        free(titlebar->elements[i]);
    }
    titlebar->element_count = 0;
    
    // Create Windows-style buttons (right side)
    int btn_size = config.decor.button_size;
    int spacing = config.decor.button_spacing;
    int x = titlebar->width - spacing - btn_size;
    int y = (config.decor.height - btn_size) / 2;
    
    titlebar->btn_close = titlebar_button_create(titlebar, x, y,
        config.decor.btn_close_color, NULL);
    
    x -= spacing + btn_size;
    titlebar->btn_max = titlebar_button_create(titlebar, x, y,
        config.decor.btn_max_color, NULL);
    
    x -= spacing + btn_size;
    titlebar->btn_min = titlebar_button_create(titlebar, x, y,
        config.decor.btn_min_color, NULL);
}

void titlebar_apply_preset_macos(struct titlebar *titlebar) {
    if (!titlebar) return;
    
    // Clear existing elements
    for (int i = 0; i < titlebar->element_count; i++) {
        if (titlebar->elements[i] && titlebar->elements[i]->rect) {
            wlr_scene_node_destroy(&titlebar->elements[i]->rect->node);
        }
        free(titlebar->elements[i]);
    }
    titlebar->element_count = 0;
    
    // Create macOS-style buttons (left side)
    int btn_size = config.decor.button_size;
    int spacing = config.decor.button_spacing;
    int x = spacing;
    int y = (config.decor.height - btn_size) / 2;
    
    titlebar->btn_close = titlebar_button_create(titlebar, x, y,
        config.decor.btn_close_color, NULL);
    
    x += spacing + btn_size;
    titlebar->btn_min = titlebar_button_create(titlebar, x, y,
        config.decor.btn_min_color, NULL);
    
    x += spacing + btn_size;
    titlebar->btn_max = titlebar_button_create(titlebar, x, y,
        config.decor.btn_max_color, NULL);
}

void titlebar_apply_preset_minimal(struct titlebar *titlebar) {
    if (!titlebar) return;
    
    // Clear existing elements
    for (int i = 0; i < titlebar->element_count; i++) {
        if (titlebar->elements[i] && titlebar->elements[i]->rect) {
            wlr_scene_node_destroy(&titlebar->elements[i]->rect->node);
        }
        free(titlebar->elements[i]);
    }
    titlebar->element_count = 0;
    
    // Just a close button on the right
    int btn_size = config.decor.button_size;
    int spacing = config.decor.button_spacing;
    int x = titlebar->width - spacing - btn_size;
    int y = (config.decor.height - btn_size) / 2;
    
    titlebar->btn_close = titlebar_button_create(titlebar, x, y,
        config.decor.btn_close_color, NULL);
}

// Destroy all elements
void titlebar_elements_destroy(struct titlebar *titlebar) {
    if (!titlebar) return;
    
    for (int i = 0; i < titlebar->element_count; i++) {
        if (titlebar->elements[i]) {
            if (titlebar->elements[i]->rect) {
                wlr_scene_node_destroy(&titlebar->elements[i]->rect->node);
            }
            free(titlebar->elements[i]);
        }
    }
    titlebar->element_count = 0;
}
