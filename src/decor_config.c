/* enhanced_config_parser.c - Parse extended decoration settings */
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Extended decoration config structure */
struct extended_decor_config {
    /* Existing config.h fields */
    bool enabled;
    int height;
    int button_size;
    int button_spacing;
    int corner_radius;
    uint32_t bg_color;
    uint32_t bg_color_inactive;
    uint32_t title_color;
    uint32_t title_color_inactive;
    uint32_t btn_close_color;
    uint32_t btn_close_hover;
    uint32_t btn_max_color;
    uint32_t btn_max_hover;
    uint32_t btn_min_color;
    uint32_t btn_min_hover;
    char font[64];
    int font_size;
    bool buttons_left;
    
    /* Extended features */
    bool glow_enabled;
    float glow_intensity;
    uint32_t glow_color;
    int glow_radius;
    
    bool gradient_enabled;
    char gradient_type[32];
    uint32_t gradient_start;
    uint32_t gradient_end;
    
    bool shadow_enabled;
    int shadow_blur;
    int shadow_offset_x;
    int shadow_offset_y;
    uint32_t shadow_color;
    
    bool blur_enabled;
    float transparency;
    
    char button_shape[32];
    bool rounded_buttons;
    
    int border_width;
    uint32_t border_color;
    int border_radius;
};

static struct extended_decor_config ext_decor = {0};

/* Parse extended decoration settings */
void parse_decoration_extended(toml_table_t *decor) {
    if (!decor) return;
    
    toml_datum_t v;
    
    /* Standard fields (already in config.h) */
    v = toml_bool_in(decor, "enabled");
    if (v.ok) config.decor.enabled = v.u.b;
    
    v = toml_int_in(decor, "height");
    if (v.ok) config.decor.height = v.u.i;
    
    v = toml_int_in(decor, "button_size");
    if (v.ok) config.decor.button_size = v.u.i;
    
    v = toml_int_in(decor, "button_spacing");
    if (v.ok) config.decor.button_spacing = v.u.i;
    
    v = toml_int_in(decor, "corner_radius");
    if (v.ok) config.decor.corner_radius = v.u.i;
    
    v = toml_string_in(decor, "bg_color");
    if (v.ok) { config.decor.bg_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "bg_color_inactive");
    if (v.ok) { config.decor.bg_color_inactive = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "title_color");
    if (v.ok) { config.decor.title_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "title_color_inactive");
    if (v.ok) { config.decor.title_color_inactive = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "close_color");
    if (v.ok) { config.decor.btn_close_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "close_hover");
    if (v.ok) { config.decor.btn_close_hover = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "maximize_color");
    if (v.ok) { config.decor.btn_max_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "maximize_hover");
    if (v.ok) { config.decor.btn_max_hover = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "minimize_color");
    if (v.ok) { config.decor.btn_min_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "minimize_hover");
    if (v.ok) { config.decor.btn_min_hover = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "font");
    if (v.ok) { 
        strncpy(config.decor.font, v.u.s, sizeof(config.decor.font) - 1);
        free(v.u.s); 
    }
    
    v = toml_int_in(decor, "font_size");
    if (v.ok) config.decor.font_size = v.u.i;
    
    v = toml_bool_in(decor, "buttons_left");
    if (v.ok) config.decor.buttons_left = v.u.b;
    
    /* Extended features - glow */
    v = toml_bool_in(decor, "glow_enabled");
    if (v.ok) ext_decor.glow_enabled = v.u.b;
    
    v = toml_double_in(decor, "glow_intensity");
    if (v.ok) ext_decor.glow_intensity = v.u.d;
    
    v = toml_string_in(decor, "glow_color");
    if (v.ok) { ext_decor.glow_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_int_in(decor, "glow_radius");
    if (v.ok) ext_decor.glow_radius = v.u.i;
    
    /* Gradient */
    v = toml_bool_in(decor, "gradient_enabled");
    if (v.ok) ext_decor.gradient_enabled = v.u.b;
    
    v = toml_string_in(decor, "gradient_type");
    if (v.ok) { 
        strncpy(ext_decor.gradient_type, v.u.s, sizeof(ext_decor.gradient_type) - 1);
        free(v.u.s); 
    }
    
    v = toml_string_in(decor, "gradient_start");
    if (v.ok) { ext_decor.gradient_start = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_string_in(decor, "gradient_end");
    if (v.ok) { ext_decor.gradient_end = parse_color(v.u.s); free(v.u.s); }
    
    /* Shadow */
    v = toml_bool_in(decor, "shadow_enabled");
    if (v.ok) ext_decor.shadow_enabled = v.u.b;
    
    v = toml_int_in(decor, "shadow_blur");
    if (v.ok) ext_decor.shadow_blur = v.u.i;
    
    v = toml_int_in(decor, "shadow_offset_x");
    if (v.ok) ext_decor.shadow_offset_x = v.u.i;
    
    v = toml_int_in(decor, "shadow_offset_y");
    if (v.ok) ext_decor.shadow_offset_y = v.u.i;
    
    v = toml_string_in(decor, "shadow_color");
    if (v.ok) { ext_decor.shadow_color = parse_color(v.u.s); free(v.u.s); }
    
    /* Blur/Transparency */
    v = toml_bool_in(decor, "blur_enabled");
    if (v.ok) ext_decor.blur_enabled = v.u.b;
    
    v = toml_double_in(decor, "transparency");
    if (v.ok) ext_decor.transparency = v.u.d;
    
    /* Button shape */
    v = toml_string_in(decor, "button_shape");
    if (v.ok) { 
        strncpy(ext_decor.button_shape, v.u.s, sizeof(ext_decor.button_shape) - 1);
        free(v.u.s); 
    }
    
    v = toml_bool_in(decor, "rounded_buttons");
    if (v.ok) ext_decor.rounded_buttons = v.u.b;
    
    /* Border */
    v = toml_int_in(decor, "border_width");
    if (v.ok) ext_decor.border_width = v.u.i;
    
    v = toml_string_in(decor, "border_color");
    if (v.ok) { ext_decor.border_color = parse_color(v.u.s); free(v.u.s); }
    
    v = toml_int_in(decor, "border_radius");
    if (v.ok) ext_decor.border_radius = v.u.i;
    
    printf("Loaded extended decoration config:\n");
    printf("  Height: %d\n", config.decor.height);
    printf("  Button size: %d\n", config.decor.button_size);
    printf("  Corner radius: %d\n", config.decor.corner_radius);
    printf("  Buttons left: %s\n", config.decor.buttons_left ? "yes" : "no");
    printf("  Glow enabled: %s\n", ext_decor.glow_enabled ? "yes" : "no");
    printf("  Gradient enabled: %s\n", ext_decor.gradient_enabled ? "yes" : "no");
    printf("  Shadow enabled: %s\n", ext_decor.shadow_enabled ? "yes" : "no");
}

/* Get extended config */
struct extended_decor_config *get_extended_decor_config(void) {
    return &ext_decor;
}

/* Apply config to titlebar theme */
void apply_config_to_theme(struct titlebar_theme *theme) {
    if (!theme) return;
    
    /* Copy basic settings */
    theme->height = config.decor.height;
    theme->padding_left = 10;
    theme->padding_right = 10;
    theme->corner_radius_top = config.decor.corner_radius;
    
    /* Colors */
    theme->bg_color = color_from_hex(config.decor.bg_color);
    theme->bg_color_inactive = color_from_hex(config.decor.bg_color_inactive);
    
    /* Apply gradient if enabled */
    if (ext_decor.gradient_enabled) {
        theme->bg_gradient.direction = GRADIENT_VERTICAL;
        if (ext_decor.gradient_start) {
            theme->bg_gradient.start = color_from_hex(ext_decor.gradient_start);
        }
        if (ext_decor.gradient_end) {
            theme->bg_gradient.end = color_from_hex(ext_decor.gradient_end);
        }
    } else {
        theme->bg_gradient.direction = GRADIENT_NONE;
    }
    
    /* Apply shadow if enabled */
    if (ext_decor.shadow_enabled) {
        theme->shadow.enabled = true;
        theme->shadow.blur = ext_decor.shadow_blur > 0 ? ext_decor.shadow_blur : 8;
        theme->shadow.offset_x = ext_decor.shadow_offset_x;
        theme->shadow.offset_y = ext_decor.shadow_offset_y;
        if (ext_decor.shadow_color) {
            theme->shadow.color = color_from_hex(ext_decor.shadow_color);
        } else {
            theme->shadow.color = color_from_hex(0x00000020);
        }
    }
    
    /* Title style */
    strncpy(theme->title.font_family, config.decor.font, sizeof(theme->title.font_family) - 1);
    theme->title.font_size = config.decor.font_size;
    theme->title.font_weight = 600;
    theme->title.color = color_from_hex(config.decor.title_color);
    theme->title_inactive.color = color_from_hex(config.decor.title_color_inactive);
    theme->title_align = ALIGN_CENTER;
    
    /* Button positioning */
    theme->buttons_left = config.decor.buttons_left;
    theme->button_spacing = config.decor.button_spacing;
    theme->button_margin = config.decor.button_spacing;
    
    /* Button styles */
    theme->btn_close.type = BTN_TYPE_CLOSE;
    theme->btn_close.shape = SHAPE_CIRCLE; // Could be configurable
    theme->btn_close.width = config.decor.button_size;
    theme->btn_close.height = config.decor.button_size;
    
    theme->btn_close.normal.bg_color = color_from_hex(config.decor.btn_close_color);
    theme->btn_close.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_close.normal.icon_scale = 0.0f;
    
    theme->btn_close.hover.bg_color = color_from_hex(config.decor.btn_close_hover);
    theme->btn_close.hover.icon_color = color_from_hex(0x000000ff);
    theme->btn_close.hover.icon_scale = 0.65f;
    
    /* Apply glow to close button if enabled */
    if (ext_decor.glow_enabled) {
        theme->btn_close.normal.shadow.enabled = true;
        theme->btn_close.normal.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius : 4;
        theme->btn_close.normal.shadow.color = color_from_hex(config.decor.btn_close_color & 0xFFFFFF40);
        
        theme->btn_close.hover.shadow.enabled = true;
        theme->btn_close.hover.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius * 2 : 8;
        theme->btn_close.hover.shadow.color = color_from_hex(config.decor.btn_close_color & 0xFFFFFF60);
    }
    
    /* Maximize button */
    theme->btn_maximize.type = BTN_TYPE_MAXIMIZE;
    theme->btn_maximize.shape = SHAPE_CIRCLE;
    theme->btn_maximize.width = config.decor.button_size;
    theme->btn_maximize.height = config.decor.button_size;
    
    theme->btn_maximize.normal.bg_color = color_from_hex(config.decor.btn_max_color);
    theme->btn_maximize.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_maximize.normal.icon_scale = 0.0f;
    
    theme->btn_maximize.hover.bg_color = color_from_hex(config.decor.btn_max_hover);
    theme->btn_maximize.hover.icon_color = color_from_hex(0x000000ff);
    theme->btn_maximize.hover.icon_scale = 0.6f;
    
    if (ext_decor.glow_enabled) {
        theme->btn_maximize.normal.shadow.enabled = true;
        theme->btn_maximize.normal.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius : 4;
        theme->btn_maximize.normal.shadow.color = color_from_hex(config.decor.btn_max_color & 0xFFFFFF40);
        
        theme->btn_maximize.hover.shadow.enabled = true;
        theme->btn_maximize.hover.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius * 2 : 8;
        theme->btn_maximize.hover.shadow.color = color_from_hex(config.decor.btn_max_color & 0xFFFFFF60);
    }
    
    /* Minimize button */
    theme->btn_minimize.type = BTN_TYPE_MINIMIZE;
    theme->btn_minimize.shape = SHAPE_CIRCLE;
    theme->btn_minimize.width = config.decor.button_size;
    theme->btn_minimize.height = config.decor.button_size;
    
    theme->btn_minimize.normal.bg_color = color_from_hex(config.decor.btn_min_color);
    theme->btn_minimize.normal.icon_color = color_from_hex(0x00000000);
    theme->btn_minimize.normal.icon_scale = 0.0f;
    
    theme->btn_minimize.hover.bg_color = color_from_hex(config.decor.btn_min_hover);
    theme->btn_minimize.hover.icon_color = color_from_hex(0x000000ff);
    theme->btn_minimize.hover.icon_scale = 0.6f;
    
    if (ext_decor.glow_enabled) {
        theme->btn_minimize.normal.shadow.enabled = true;
        theme->btn_minimize.normal.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius : 4;
        theme->btn_minimize.normal.shadow.color = color_from_hex(config.decor.btn_min_color & 0xFFFFFF40);
        
        theme->btn_minimize.hover.shadow.enabled = true;
        theme->btn_minimize.hover.shadow.blur = ext_decor.glow_radius > 0 ? ext_decor.glow_radius * 2 : 8;
        theme->btn_minimize.hover.shadow.color = color_from_hex(config.decor.btn_min_color & 0xFFFFFF60);
    }
    
    theme->inactive_opacity = 0.85f;
    
    printf("Applied config to titlebar theme\n");
}
