/* visual_themes.c - Theme presets */
#define _POSIX_C_SOURCE 200809L

#include "../visual_types.h"
#include <string.h>
#include <stddef.h>

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

rgba_t rgba_hex(uint32_t hex) {
    rgba_t c;
    c.r = ((hex >> 24) & 0xff) / 255.0f;
    c.g = ((hex >> 16) & 0xff) / 255.0f;
    c.b = ((hex >> 8) & 0xff) / 255.0f;
    c.a = (hex & 0xff) / 255.0f;
    return c;
}

border_t border_solid(int width, rgba_t color, int radius) {
    border_t b;
    b.width = width;
    b.color = color;
    b.radius = radius;
    return b;
}

border_t border_none(void) {
    return border_solid(0, (rgba_t){0,0,0,0}, 0);
}

shadow_t shadow_soft(int radius, int offset, rgba_t color) {
    shadow_t s;
    s.enabled = true;
    s.radius = radius;
    s.offset_x = offset;
    s.offset_y = offset;
    s.color = color;
    return s;
}

shadow_t shadow_none(void) {
    shadow_t s;
    s.enabled = false;
    s.radius = 0;
    s.offset_x = 0;
    s.offset_y = 0;
    s.color = (rgba_t){0,0,0,0};
    return s;
}

shadow_t shadow_inset(int radius, rgba_t color) {
    shadow_t s;
    s.enabled = true;
    s.radius = radius;
    s.offset_x = 0;
    s.offset_y = 0;
    s.color = color;
    return s;
}

gradient_t gradient_simple(gradient_type_t type, rgba_t start, rgba_t end) {
    gradient_t g;
    g.type = type;
    g.start = start;
    g.end = end;
    return g;
}

text_style_t text_style_create(const char *font, int size, font_weight_t weight, rgba_t color) {
    text_style_t t;
    strncpy(t.font_family, font, sizeof(t.font_family) - 1);
    t.font_family[sizeof(t.font_family) - 1] = '\0';
    t.font_size = size;
    t.font_weight = weight;
    t.color = color;
    return t;
}

button_style_t button_style_circle(int size, rgba_t color, icon_type_t icon) {
    (void)icon;
    button_style_t b;
    memset(&b, 0, sizeof(b));
    b.width = size;
    b.height = size;
    for (int i = 0; i < 4; i++) {
        b.states[i].bg_color = color;
        b.states[i].icon.color = (rgba_t){1, 1, 1, 1};
        b.states[i].icon.scale = 0.7f;
    }
    return b;
}

button_style_t button_style_rect(int width, int height, rgba_t color, icon_type_t icon) {
    (void)icon;
    button_style_t b;
    memset(&b, 0, sizeof(b));
    b.width = width;
    b.height = height;
    for (int i = 0; i < 4; i++) {
        b.states[i].bg_color = (rgba_t){0, 0, 0, 0};
        b.states[i].icon.color = color;
        b.states[i].icon.scale = 0.65f;
    }
    return b;
}

/* ============================================================================
 * COLOR PALETTES
 * ============================================================================ */

/* Catppuccin Mocha */
static const struct {
    uint32_t rosewater, flamingo, pink, mauve, red, maroon, peach, yellow;
    uint32_t green, teal, sky, sapphire, blue, lavender, text, subtext1;
    uint32_t subtext0, overlay2, overlay1, overlay0, surface2, surface1;
    uint32_t surface0, base, mantle, crust;
} catppuccin_mocha = {
    .rosewater = 0xf5e0dcff, .flamingo = 0xf2cdcdff, .pink = 0xf5c2e7ff,
    .mauve = 0xcba6f7ff, .red = 0xf38ba8ff, .maroon = 0xeba0acff,
    .peach = 0xfab387ff, .yellow = 0xf9e2afff, .green = 0xa6e3a1ff,
    .teal = 0x94e2d5ff, .sky = 0x89dcebff, .sapphire = 0x74c7ecff,
    .blue = 0x89b4faff, .lavender = 0xb4befeff, .text = 0xcdd6f4ff,
    .subtext1 = 0xbac2deff, .subtext0 = 0xa6adc8ff, .overlay2 = 0x9399b2ff,
    .overlay1 = 0x7f849cff, .overlay0 = 0x6c7086ff, .surface2 = 0x585b70ff,
    .surface1 = 0x45475aff, .surface0 = 0x313244ff, .base = 0x1e1e2eff,
    .mantle = 0x181825ff, .crust = 0x11111bff,
};

/* Catppuccin Latte */
static const struct {
    uint32_t rosewater, flamingo, pink, mauve, red, maroon, peach, yellow;
    uint32_t green, teal, sky, sapphire, blue, lavender, text, subtext1;
    uint32_t subtext0, overlay2, overlay1, overlay0, surface2, surface1;
    uint32_t surface0, base, mantle, crust;
} catppuccin_latte = {
    .rosewater = 0xdc8a78ff, .flamingo = 0xdd7878ff, .pink = 0xea76cbff,
    .mauve = 0x8839efff, .red = 0xd20f39ff, .maroon = 0xe64553ff,
    .peach = 0xfe640bff, .yellow = 0xdf8e1dff, .green = 0x40a02bff,
    .teal = 0x179299ff, .sky = 0x04a5e5ff, .sapphire = 0x209fb5ff,
    .blue = 0x1e66f5ff, .lavender = 0x7287fdff, .text = 0x4c4f69ff,
    .subtext1 = 0x5c5f77ff, .subtext0 = 0x6c6f85ff, .overlay2 = 0x7c7f93ff,
    .overlay1 = 0x8c8fa1ff, .overlay0 = 0x9ca0b0ff, .surface2 = 0xacb0beff,
    .surface1 = 0xbcc0ccff, .surface0 = 0xccd0daff, .base = 0xeff1f5ff,
    .mantle = 0xe6e9efff, .crust = 0xdce0e8ff,
};

/* Nord */
static const struct {
    uint32_t polar0, polar1, polar2, polar3;
    uint32_t snow0, snow1, snow2;
    uint32_t frost0, frost1, frost2, frost3;
    uint32_t aurora0, aurora1, aurora2, aurora3, aurora4;
} nord = {
    .polar0 = 0x2e3440ff, .polar1 = 0x3b4252ff, .polar2 = 0x434c5eff, .polar3 = 0x4c566aff,
    .snow0 = 0xd8dee9ff, .snow1 = 0xe5e9f0ff, .snow2 = 0xeceff4ff,
    .frost0 = 0x8fbcbbff, .frost1 = 0x88c0d0ff, .frost2 = 0x81a1c1ff, .frost3 = 0x5e81acff,
    .aurora0 = 0xbf616aff, .aurora1 = 0xd08770ff, .aurora2 = 0xebcb8bff,
    .aurora3 = 0xa3be8cff, .aurora4 = 0xb48eadff,
};

/* Dracula */
static const struct {
    uint32_t bg, current, fg, comment, cyan, green, orange, pink, purple, red, yellow;
} dracula = {
    .bg = 0x282a36ff, .current = 0x44475aff, .fg = 0xf8f8f2ff,
    .comment = 0x6272a4ff, .cyan = 0x8be9fdff, .green = 0x50fa7bff,
    .orange = 0xffb86cff, .pink = 0xff79c6ff, .purple = 0xbd93f9ff,
    .red = 0xff5555ff, .yellow = 0xf1fa8cff,
};

/* Gruvbox Dark */
static const struct {
    uint32_t bg, bg1, bg2, bg3, fg, fg1, red, green, yellow, blue, purple, aqua, orange;
} gruvbox = {
    .bg = 0x282828ff, .bg1 = 0x3c3836ff, .bg2 = 0x504945ff, .bg3 = 0x665c54ff,
    .fg = 0xebdbb2ff, .fg1 = 0xd5c4a1ff, .red = 0xfb4934ff, .green = 0xb8bb26ff,
    .yellow = 0xfabd2fff, .blue = 0x83a598ff, .purple = 0xd3869bff,
    .aqua = 0x8ec07cff, .orange = 0xfe8019ff,
};

/* Tokyo Night */
static const struct {
    uint32_t bg, bg_dark, bg_highlight, terminal_black, fg, fg_dark, fg_gutter;
    uint32_t comment, cyan, blue, blue1, blue2, magenta, purple, orange, yellow, green, teal, red;
} tokyo_night = {
    .bg = 0x1a1b26ff, .bg_dark = 0x16161eff, .bg_highlight = 0x292e42ff,
    .terminal_black = 0x414868ff, .fg = 0xc0caf5ff, .fg_dark = 0xa9b1d6ff,
    .fg_gutter = 0x3b4261ff, .comment = 0x565f89ff, .cyan = 0x7dcfffff,
    .blue = 0x7aa2f7ff, .blue1 = 0x2ac3deff, .blue2 = 0x0db9d7ff,
    .magenta = 0xbb9af7ff, .purple = 0x9d7cd8ff, .orange = 0xff9e64ff,
    .yellow = 0xe0af68ff, .green = 0x9ece6aff, .teal = 0x1abc9cff, .red = 0xf7768eff,
};

/* One Dark */
static const struct {
    uint32_t bg, bg_light, fg, gray, red, green, yellow, blue, purple, cyan;
} one_dark = {
    .bg = 0x282c34ff, .bg_light = 0x3e4451ff, .fg = 0xabb2bfff, .gray = 0x5c6370ff,
    .red = 0xe06c75ff, .green = 0x98c379ff, .yellow = 0xe5c07bff, .blue = 0x61afefff,
    .purple = 0xc678ddff, .cyan = 0x56b6c2ff,
};

/* ============================================================================
 * DEFAULT CONFIG
 * ============================================================================ */

visual_config_t visual_config_default(void) {
    visual_config_t cfg = {0};
    
    cfg.decoration.enabled = true;
    
    cfg.decoration.window_border = border_solid(1, rgba_hex(0x45475aff), 0);
    cfg.decoration.window_border_inactive = border_solid(1, rgba_hex(0x313244ff), 0);
    cfg.decoration.window_border_focused = border_solid(2, rgba_hex(0x89b4faff), 0);
    
    cfg.decoration.window_shadow = shadow_soft(20, 5, rgba_hex(0x00000060));
    cfg.decoration.window_shadow_inactive = shadow_soft(10, 2, rgba_hex(0x00000040));
    cfg.decoration.window_shadow_maximized = shadow_none();
    
    cfg.decoration.resize_handle_size = 5;
    cfg.decoration.resize_handle_color = rgba_hex(0x00000000);
    cfg.decoration.resize_handle_visible = false;
    
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    tb->height = 32;
    tb->padding_left = 10;
    tb->padding_right = 10;
    tb->padding_top = 0;
    tb->padding_bottom = 0;
    
    tb->bg_color = rgba_hex(catppuccin_mocha.base);
    tb->bg_color_inactive = rgba_hex(catppuccin_mocha.mantle);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V,
        rgba_hex(catppuccin_mocha.surface0),
        rgba_hex(catppuccin_mocha.base));
    tb->bg_gradient_inactive = gradient_simple(GRAD_NONE, rgba_hex(0), rgba_hex(0));
    
    tb->border = border_none();
    tb->shadow = shadow_none();
    tb->inner_shadow = shadow_inset(3, rgba_hex(0x00000020));
    
    tb->corner_radius_tl = 10;
    tb->corner_radius_tr = 10;
    tb->corner_radius_br = 0;
    tb->corner_radius_bl = 0;
    
    tb->title_style = text_style_create("sans-serif", 12, FONT_WEIGHT_MEDIUM,
                                         rgba_hex(catppuccin_mocha.text));
    tb->title_style_inactive = text_style_create("sans-serif", 12, FONT_WEIGHT_MEDIUM,
                                                  rgba_hex(catppuccin_mocha.overlay0));
    tb->title_align = TEXT_ALIGN_CENTER;
    tb->title_max_width = 0;
    
    tb->buttons_visible = true;
    tb->buttons_left = false;
    tb->button_spacing = 8;
    tb->button_margin = 8;
    
    tb->btn_close = button_style_circle(14, rgba_hex(catppuccin_mocha.red), ICON_CLOSE);
    tb->btn_close.states[1].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_close.states[2].icon.color = rgba_hex(catppuccin_mocha.crust);
    
    tb->btn_maximize = button_style_circle(14, rgba_hex(catppuccin_mocha.green), ICON_MAXIMIZE);
    tb->btn_maximize.states[1].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_maximize.states[2].icon.color = rgba_hex(catppuccin_mocha.crust);
    
    tb->btn_minimize = button_style_circle(14, rgba_hex(catppuccin_mocha.yellow), ICON_MINIMIZE);
    tb->btn_minimize.states[1].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_minimize.states[2].icon.color = rgba_hex(catppuccin_mocha.crust);
    
    tb->button_order[0] = 2;
    tb->button_order[1] = 1;
    tb->button_order[2] = 0;
    tb->button_order[3] = -1;
    
    tb->inactive_opacity = 0.8f;
    tb->separator_visible = false;
    tb->transition_ms = 150;
    
    cfg.effects.focus_ring_enabled = false;
    cfg.effects.glow_enabled = false;
    cfg.effects.backdrop_blur_enabled = false;
    cfg.effects.vibrancy_enabled = false;
    
    cfg.workspace_indicator.enabled = false;
    
    cfg.animations_enabled = true;
    cfg.animation_duration_ms = 200;
    cfg.animation_curve = 0.25f;
    cfg.font_antialias = true;
    cfg.font_subpixel = true;
    cfg.subpixel_order = SUBPIXEL_RGB;
    cfg.font_hinting = HINT_SLIGHT;
    cfg.scale_factor = 1.0f;
    cfg.dpi = 96;
    
    return cfg;
}



static visual_config_t theme_macos_light(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 28;
    tb->bg_color = rgba_hex(0xe8e8e8ff);
    tb->bg_color_inactive = rgba_hex(0xf6f6f6ff);
    tb->buttons_left = true;
    
    return cfg;
}

visual_config_t visual_config_preset(theme_preset_t preset) {
    switch (preset) {
    case THEME_MACOS_LIGHT: 
        return theme_macos_light();
    case THEME_DEFAULT:
    default: 
        return visual_config_default();
    }
}
