/* visual_themes.c - Theme presets */
#define _POSIX_C_SOURCE 200809L

#include "../visual.h"
#include <string.h>

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
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_close.states[BTN_STATE_PRESSED].icon.color = rgba_hex(catppuccin_mocha.crust);
    
    tb->btn_maximize = button_style_circle(14, rgba_hex(catppuccin_mocha.green), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_maximize.states[BTN_STATE_PRESSED].icon.color = rgba_hex(catppuccin_mocha.crust);
    
    tb->btn_minimize = button_style_circle(14, rgba_hex(catppuccin_mocha.yellow), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(catppuccin_mocha.crust);
    tb->btn_minimize.states[BTN_STATE_PRESSED].icon.color = rgba_hex(catppuccin_mocha.crust);
    
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

/* ============================================================================
 * THEME PRESETS
 * ============================================================================ */

static visual_config_t theme_macos_light(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 28;
    tb->bg_color = rgba_hex(0xe8e8e8ff);
    tb->bg_color_inactive = rgba_hex(0xf6f6f6ff);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0xfafafaff), rgba_hex(0xe0e0e0ff));
    
    tb->corner_radius_tl = 10;
    tb->corner_radius_tr = 10;
    
    tb->title_style = text_style_create("SF Pro Display, Helvetica Neue, sans-serif",
                                         13, FONT_WEIGHT_MEDIUM, rgba_hex(0x000000ff));
    tb->title_style_inactive = text_style_create("SF Pro Display, Helvetica Neue, sans-serif",
                                                  13, FONT_WEIGHT_MEDIUM, rgba_hex(0x00000080));
    tb->title_align = TEXT_ALIGN_CENTER;
    
    tb->buttons_left = true;
    tb->button_spacing = 8;
    tb->button_margin = 8;
    
    tb->btn_close = button_style_circle(12, rgba_hex(0xff5f57ff), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x00000000);
    tb->btn_close.states[BTN_STATE_HOVER].bg_color = rgba_hex(0xff5f57ff);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(0x4d0000ff);
    tb->btn_close.states[BTN_STATE_PRESSED].bg_color = rgba_hex(0xbf4942ff);
    
    tb->btn_minimize = button_style_circle(12, rgba_hex(0xfebc2eff), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x00000000);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(0x995700ff);
    
    tb->btn_maximize = button_style_circle(12, rgba_hex(0x28c840ff), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x00000000);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(0x006500ff);
    
    tb->button_order[0] = 0;
    tb->button_order[1] = 2;
    tb->button_order[2] = 1;
    tb->button_order[3] = -1;
    
    cfg.decoration.window_shadow = shadow_soft(25, 8, rgba_hex(0x00000050));
    
    return cfg;
}

static visual_config_t theme_macos_dark(void) {
    visual_config_t cfg = theme_macos_light();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->bg_color = rgba_hex(0x3c3c3cff);
    tb->bg_color_inactive = rgba_hex(0x2d2d2dff);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0x4a4a4aff), rgba_hex(0x323232ff));
    
    tb->title_style.color = rgba_hex(0xffffffff);
    tb->title_style_inactive.color = rgba_hex(0xffffff80);
    
    return cfg;
}

static visual_config_t theme_windows_11(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 32;
    tb->padding_left = 12;
    tb->padding_right = 0;
    tb->bg_color = rgba_hex(0xf3f3f3ff);
    tb->bg_color_inactive = rgba_hex(0xfbfbfbff);
    tb->bg_gradient.type = GRAD_NONE;
    
    tb->corner_radius_tl = 8;
    tb->corner_radius_tr = 8;
    
    tb->title_style = text_style_create("Segoe UI Variable, Segoe UI, sans-serif",
                                         12, FONT_WEIGHT_NORMAL, rgba_hex(0x000000ff));
    tb->title_align = TEXT_ALIGN_LEFT;
    
    tb->buttons_left = false;
    tb->button_spacing = 0;
    tb->button_margin = 0;
    
    tb->btn_close = button_style_rect(46, 32, rgba_hex(0x000000ff), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].bg_color = rgba_hex(0xe81123ff);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(0xffffffff);
    tb->btn_close.states[BTN_STATE_PRESSED].bg_color = rgba_hex(0xf1707aff);
    
    tb->btn_maximize = button_style_rect(46, 32, rgba_hex(0x000000ff), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].bg_color = rgba_hex(0x0000001a);
    
    tb->btn_minimize = button_style_rect(46, 32, rgba_hex(0x000000ff), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].bg_color = rgba_hex(0x0000001a);
    
    tb->button_order[0] = 2;
    tb->button_order[1] = 1;
    tb->button_order[2] = 0;
    tb->button_order[3] = -1;
    
    cfg.decoration.window_shadow = shadow_soft(15, 3, rgba_hex(0x00000030));
    cfg.decoration.window_border = border_solid(1, rgba_hex(0x00000010), 8);
    
    return cfg;
}

static visual_config_t theme_gnome_adwaita(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 36;
    tb->padding_left = 10;
    tb->padding_right = 10;
    tb->bg_color = rgba_hex(0xebebefff);
    tb->bg_color_inactive = rgba_hex(0xf6f5f4ff);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0xf6f5f4ff), rgba_hex(0xdedddbff));
    
    tb->corner_radius_tl = 12;
    tb->corner_radius_tr = 12;
    
    tb->title_style = text_style_create("Cantarell, sans-serif", 11, FONT_WEIGHT_BOLD, rgba_hex(0x000000e6));
    tb->title_align = TEXT_ALIGN_CENTER;
    
    tb->buttons_left = false;
    tb->button_spacing = 6;
    tb->button_margin = 6;
    
    tb->btn_close = button_style_circle(22, rgba_hex(0x00000000), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x000000b3);
    tb->btn_close.states[BTN_STATE_HOVER].bg_color = rgba_hex(0xe0404080);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(0xffffffff);
    
    tb->btn_maximize = button_style_circle(22, rgba_hex(0x00000000), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x000000b3);
    tb->btn_maximize.states[BTN_STATE_HOVER].bg_color = rgba_hex(0x00000020);
    
    tb->btn_minimize = button_style_circle(22, rgba_hex(0x00000000), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0x000000b3);
    tb->btn_minimize.states[BTN_STATE_HOVER].bg_color = rgba_hex(0x00000020);
    
    cfg.decoration.window_shadow = shadow_soft(20, 6, rgba_hex(0x00000040));
    
    return cfg;
}

static visual_config_t theme_gnome_adwaita_dark(void) {
    visual_config_t cfg = theme_gnome_adwaita();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->bg_color = rgba_hex(0x303030ff);
    tb->bg_color_inactive = rgba_hex(0x404040ff);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0x404040ff), rgba_hex(0x282828ff));
    
    tb->title_style.color = rgba_hex(0xffffffef);
    tb->title_style_inactive.color = rgba_hex(0xffffff80);
    
    tb->btn_close.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    tb->btn_maximize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    tb->btn_minimize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    
    return cfg;
}

static visual_config_t theme_minimal(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 24;
    tb->padding_left = 8;
    tb->padding_right = 8;
    tb->bg_color = rgba_hex(0xffffffff);
    tb->bg_color_inactive = rgba_hex(0xf5f5f5ff);
    tb->bg_gradient.type = GRAD_NONE;
    
    tb->corner_radius_tl = 0;
    tb->corner_radius_tr = 0;
    
    tb->border = border_solid(1, rgba_hex(0x00000020), 0);
    
    tb->title_style = text_style_create("Inter, SF Pro Display, sans-serif", 11, FONT_WEIGHT_NORMAL, rgba_hex(0x000000cc));
    tb->title_align = TEXT_ALIGN_LEFT;
    
    tb->buttons_left = false;
    tb->button_spacing = 2;
    tb->button_margin = 4;
    
    tb->btn_close = button_style_rect(20, 20, rgba_hex(0x000000cc), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(0xff0000ff);
    
    tb->btn_maximize = button_style_rect(20, 20, rgba_hex(0x000000cc), ICON_MAXIMIZE);
    tb->btn_minimize = button_style_rect(20, 20, rgba_hex(0x000000cc), ICON_MINIMIZE);
    
    cfg.decoration.window_shadow = shadow_soft(8, 2, rgba_hex(0x00000030));
    cfg.decoration.window_border = border_solid(1, rgba_hex(0x00000015), 0);
    
    return cfg;
}

static visual_config_t theme_minimal_dark(void) {
    visual_config_t cfg = theme_minimal();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->bg_color = rgba_hex(0x1a1a1aff);
    tb->bg_color_inactive = rgba_hex(0x222222ff);
    tb->border.color = rgba_hex(0xffffff15);
    
    tb->title_style.color = rgba_hex(0xffffffcc);
    
    tb->btn_close.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    tb->btn_maximize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    tb->btn_minimize.states[BTN_STATE_NORMAL].icon.color = rgba_hex(0xffffffcc);
    
    cfg.decoration.window_border.color = rgba_hex(0xffffff15);
    
    return cfg;
}

static visual_config_t theme_nord(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 30;
    tb->bg_color = rgba_hex(nord.polar1);
    tb->bg_color_inactive = rgba_hex(nord.polar0);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(nord.polar2), rgba_hex(nord.polar1));
    
    tb->corner_radius_tl = 8;
    tb->corner_radius_tr = 8;
    
    tb->title_style = text_style_create("Inter, sans-serif", 12, FONT_WEIGHT_MEDIUM, rgba_hex(nord.snow0));
    tb->title_style_inactive.color = rgba_hex(nord.polar3);
    
    tb->btn_close = button_style_circle(14, rgba_hex(nord.aurora0), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(nord.polar0);
    
    tb->btn_maximize = button_style_circle(14, rgba_hex(nord.aurora3), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(nord.polar0);
    
    tb->btn_minimize = button_style_circle(14, rgba_hex(nord.aurora2), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(nord.polar0);
    
    cfg.decoration.window_border = border_solid(1, rgba_hex(nord.polar2), 8);
    cfg.decoration.window_shadow = shadow_soft(15, 5, rgba_hex(0x00000050));
    
    return cfg;
}

static visual_config_t theme_dracula(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 30;
    tb->bg_color = rgba_hex(dracula.current);
    tb->bg_color_inactive = rgba_hex(dracula.bg);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0x4a4d62ff), rgba_hex(dracula.current));
    
    tb->corner_radius_tl = 8;
    tb->corner_radius_tr = 8;
    
    tb->title_style = text_style_create("Fira Code, monospace", 12, FONT_WEIGHT_NORMAL, rgba_hex(dracula.fg));
    tb->title_style_inactive.color = rgba_hex(dracula.comment);
    
    tb->btn_close = button_style_circle(14, rgba_hex(dracula.red), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(dracula.bg);
    
    tb->btn_maximize = button_style_circle(14, rgba_hex(dracula.green), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(dracula.bg);
    
    tb->btn_minimize = button_style_circle(14, rgba_hex(dracula.yellow), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(dracula.bg);
    
    cfg.decoration.window_border = border_solid(1, rgba_hex(dracula.comment), 8);
    cfg.decoration.window_shadow = shadow_soft(18, 6, rgba_hex(0x00000060));
    
    return cfg;
}

static visual_config_t theme_gruvbox_dark(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 28;
    tb->bg_color = rgba_hex(gruvbox.bg1);
    tb->bg_color_inactive = rgba_hex(gruvbox.bg);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(gruvbox.bg2), rgba_hex(gruvbox.bg1));
    
    tb->corner_radius_tl = 6;
    tb->corner_radius_tr = 6;
    
    tb->title_style = text_style_create("JetBrains Mono, monospace", 11, FONT_WEIGHT_NORMAL, rgba_hex(gruvbox.fg));
    tb->title_style_inactive.color = rgba_hex(gruvbox.bg3);
    
    tb->btn_close = button_style_circle(12, rgba_hex(gruvbox.red), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(gruvbox.bg);
    
    tb->btn_maximize = button_style_circle(12, rgba_hex(gruvbox.green), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(gruvbox.bg);
    
    tb->btn_minimize = button_style_circle(12, rgba_hex(gruvbox.yellow), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(gruvbox.bg);
    
    cfg.decoration.window_border = border_solid(2, rgba_hex(gruvbox.bg3), 6);
    
    return cfg;
}

static visual_config_t theme_tokyo_night(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 32;
    tb->bg_color = rgba_hex(tokyo_night.bg_highlight);
    tb->bg_color_inactive = rgba_hex(tokyo_night.bg);
    tb->bg_gradient = gradient_simple(GRAD_LINEAR_V, rgba_hex(0x363b54ff), rgba_hex(tokyo_night.bg_highlight));
    
    tb->corner_radius_tl = 10;
    tb->corner_radius_tr = 10;
    
    tb->title_style = text_style_create("JetBrains Mono, monospace", 12, FONT_WEIGHT_MEDIUM, rgba_hex(tokyo_night.fg));
    tb->title_style_inactive.color = rgba_hex(tokyo_night.comment);
    
    tb->btn_close = button_style_circle(14, rgba_hex(tokyo_night.red), ICON_CLOSE);
    tb->btn_close.states[BTN_STATE_HOVER].icon.color = rgba_hex(tokyo_night.bg);
    
    tb->btn_maximize = button_style_circle(14, rgba_hex(tokyo_night.green), ICON_MAXIMIZE);
    tb->btn_maximize.states[BTN_STATE_HOVER].icon.color = rgba_hex(tokyo_night.bg);
    
    tb->btn_minimize = button_style_circle(14, rgba_hex(tokyo_night.yellow), ICON_MINIMIZE);
    tb->btn_minimize.states[BTN_STATE_HOVER].icon.color = rgba_hex(tokyo_night.bg);
    
    cfg.effects.glow_enabled = true;
    cfg.effects.glow_color = rgba_hex(tokyo_night.blue);
    cfg.effects.glow_radius = 15;
    cfg.effects.glow_intensity = 0.3f;
    
    cfg.decoration.window_border = border_solid(1, rgba_hex(tokyo_night.terminal_black), 10);
    cfg.decoration.window_shadow = shadow_soft(20, 8, rgba_hex(0x000000a0));
    
    return cfg;
}

static visual_config_t theme_one_dark(void) {
    visual_config_t cfg = visual_config_default();
    titlebar_config_t *tb = &cfg.decoration.titlebar;
    
    tb->height = 30;
    tb->bg_color = rgba_hex(one_dark.bg_light);
    tb->bg_color_inactive = rgba_hex(one_dark.bg);
    tb->bg_gradient.type = GRAD_NONE;
    
    tb->corner_radius_tl = 6;
    tb->corner_radius_tr = 6;
    
    tb->title_style = text_style_create("Fira Code, monospace", 12, FONT_WEIGHT_NORMAL, rgba_hex(one_dark.fg));
    tb->title_style_inactive.color = rgba_hex(one_dark.gray);
    
    tb->btn_close = button_style_circle(14, rgba_hex(one_dark.red), ICON_CLOSE);
    tb->btn_maximize = button_style_circle(14, rgba_hex(one_dark.green), ICON_MAXIMIZE);
    tb->btn_minimize = button_style_circle(14, rgba_hex(one_dark.yellow), ICON_MINIMIZE);
    
    cfg.decoration.window_border = border_solid(1, rgba_hex(one_dark.gray), 6);
    
    return cfg;
}

visual_config_t visual_config_preset(theme_preset_t preset) {
    switch (preset) {
    case THEME_MACOS_LIGHT: return theme_macos_light();
    case THEME_MACOS_DARK: return theme_macos_dark();
    case THEME_WINDOWS_11: return theme_windows_11();
    case THEME_GNOME_ADWAITA: return theme_gnome_adwaita();
    case THEME_GNOME_ADWAITA_DARK: return theme_gnome_adwaita_dark();
    case THEME_MINIMAL: return theme_minimal();
    case THEME_MINIMAL_DARK: return theme_minimal_dark();
    case THEME_CATPPUCCIN_MOCHA: return visual_config_default();
    case THEME_CATPPUCCIN_LATTE: {
        visual_config_t cfg = visual_config_default();
        titlebar_config_t *tb = &cfg.decoration.titlebar;
        tb->bg_color = rgba_hex(catppuccin_latte.base);
        tb->bg_color_inactive = rgba_hex(catppuccin_latte.mantle);
        tb->bg_gradient = gradient_simple(GRAD_LINEAR_V,
            rgba_hex(catppuccin_latte.surface0), rgba_hex(catppuccin_latte.base));
        tb->title_style.color = rgba_hex(catppuccin_latte.text);
        tb->title_style_inactive.color = rgba_hex(catppuccin_latte.overlay0);
        tb->btn_close = button_style_circle(14, rgba_hex(catppuccin_latte.red), ICON_CLOSE);
        tb->btn_maximize = button_style_circle(14, rgba_hex(catppuccin_latte.green), ICON_MAXIMIZE);
        tb->btn_minimize = button_style_circle(14, rgba_hex(catppuccin_latte.yellow), ICON_MINIMIZE);
        cfg.decoration.window_border.color = rgba_hex(catppuccin_latte.surface1);
        return cfg;
    }
    case THEME_NORD: return theme_nord();
    case THEME_DRACULA: return theme_dracula();
    case THEME_GRUVBOX_DARK: return theme_gruvbox_dark();
    case THEME_TOKYO_NIGHT: return theme_tokyo_night();
    case THEME_ONE_DARK: return theme_one_dark();
    case THEME_DEFAULT:
    default: return visual_config_default();
    }
}
