/* visual_config.c - Parse visual configuration from TOML */
#define _POSIX_C_SOURCE 200809L

#include "../visual.h"
#include "../toml.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
/* Parse theme preset from string */
static theme_preset_t parse_theme_preset(const char *str) {
    if (!str) return THEME_DEFAULT;
    
    if (strcasecmp(str, "macos_light") == 0) return THEME_MACOS_LIGHT;
    if (strcasecmp(str, "macos_dark") == 0) return THEME_MACOS_DARK;
    if (strcasecmp(str, "windows_11") == 0) return THEME_WINDOWS_11;
    if (strcasecmp(str, "gnome_adwaita") == 0) return THEME_GNOME_ADWAITA;
    if (strcasecmp(str, "gnome_adwaita_dark") == 0) return THEME_GNOME_ADWAITA_DARK;
    if (strcasecmp(str, "minimal") == 0) return THEME_MINIMAL;
    if (strcasecmp(str, "minimal_dark") == 0) return THEME_MINIMAL_DARK;
    if (strcasecmp(str, "catppuccin_mocha") == 0) return THEME_CATPPUCCIN_MOCHA;
    if (strcasecmp(str, "catppuccin_latte") == 0) return THEME_CATPPUCCIN_LATTE;
    if (strcasecmp(str, "nord") == 0) return THEME_NORD;
    if (strcasecmp(str, "dracula") == 0) return THEME_DRACULA;
    if (strcasecmp(str, "gruvbox_dark") == 0) return THEME_GRUVBOX_DARK;
    if (strcasecmp(str, "tokyo_night") == 0) return THEME_TOKYO_NIGHT;
    if (strcasecmp(str, "one_dark") == 0) return THEME_ONE_DARK;
    
    return THEME_DEFAULT;
}

/* Parse icon type from string */
static icon_type_t parse_icon_type(const char *str) {
    if (!str) return ICON_NONE;
    
    if (strcasecmp(str, "close") == 0) return ICON_CLOSE;
    if (strcasecmp(str, "maximize") == 0) return ICON_MAXIMIZE;
    if (strcasecmp(str, "minimize") == 0) return ICON_MINIMIZE;
    if (strcasecmp(str, "restore") == 0) return ICON_RESTORE;
    
    return ICON_NONE;
}

/* Parse text alignment from string */
static text_align_t parse_text_align(const char *str) {
    if (!str) return TEXT_ALIGN_LEFT;
    
    if (strcasecmp(str, "left") == 0) return TEXT_ALIGN_LEFT;
    if (strcasecmp(str, "center") == 0) return TEXT_ALIGN_CENTER;
    if (strcasecmp(str, "right") == 0) return TEXT_ALIGN_RIGHT;
    
    return TEXT_ALIGN_LEFT;
}

/* Parse shape type from string */
static shape_type_t parse_shape_type(const char *str) {
    if (!str) return SHAPE_RECT;
    
    if (strcasecmp(str, "rect") == 0) return SHAPE_RECT;
    if (strcasecmp(str, "rounded_rect") == 0) return SHAPE_ROUNDED_RECT;
    if (strcasecmp(str, "circle") == 0) return SHAPE_CIRCLE;
    if (strcasecmp(str, "pill") == 0) return SHAPE_PILL;
    if (strcasecmp(str, "squircle") == 0) return SHAPE_SQUIRCLE;
    
    return SHAPE_RECT;
}

/* Parse button style from TOML table */
static void parse_button_style(toml_table_t *tbl, button_style_t *style) {
    if (!tbl || !style) return;
    
    toml_datum_t v;
    
    v = toml_string_in(tbl, "shape");
    if (v.ok) {
        style->shape = parse_shape_type(v.u.s);
        free(v.u.s);
    }
    
    v = toml_int_in(tbl, "width");
    if (v.ok) style->width = v.u.i;
    
    v = toml_int_in(tbl, "height");
    if (v.ok) style->height = v.u.i;
    
    v = toml_string_in(tbl, "bg_color");
    if (v.ok) {
        style->states[BTN_STATE_NORMAL].bg_color = rgba_hex(parse_color(v.u.s));
        free(v.u.s);
    }
    
    v = toml_string_in(tbl, "hover_color");
    if (v.ok) {
        style->states[BTN_STATE_HOVER].bg_color = rgba_hex(parse_color(v.u.s));
        free(v.u.s);
    }
    
    v = toml_string_in(tbl, "icon_type");
    if (v.ok) {
        icon_type_t icon = parse_icon_type(v.u.s);
        for (int i = 0; i < BTN_STATE_COUNT; i++) {
            style->states[i].icon.type = icon;
        }
        free(v.u.s);
    }
    
    v = toml_string_in(tbl, "icon_color");
    if (v.ok) {
        rgba_t color = rgba_hex(parse_color(v.u.s));
        for (int i = 0; i < BTN_STATE_COUNT; i++) {
            style->states[i].icon.color = color;
        }
        free(v.u.s);
    }
}

/* Parse titlebar configuration from TOML */
static void parse_titlebar_config(toml_table_t *tbl, titlebar_config_t *cfg) {
    if (!tbl || !cfg) return;
    
    toml_datum_t v;
    
    v = toml_int_in(tbl, "height");
    if (v.ok) cfg->height = v.u.i;
    
    v = toml_int_in(tbl, "padding_left");
    if (v.ok) cfg->padding_left = v.u.i;
    
    v = toml_int_in(tbl, "padding_right");
    if (v.ok) cfg->padding_right = v.u.i;
    
    v = toml_string_in(tbl, "bg_color");
    if (v.ok) {
        cfg->bg_color = rgba_hex(parse_color(v.u.s));
        free(v.u.s);
    }
    
    v = toml_string_in(tbl, "bg_color_inactive");
    if (v.ok) {
        cfg->bg_color_inactive = rgba_hex(parse_color(v.u.s));
        free(v.u.s);
    }
    
    v = toml_double_in(tbl, "corner_radius");
    if (v.ok) {
        cfg->corner_radius_tl = v.u.d;
        cfg->corner_radius_tr = v.u.d;
    }
    
    v = toml_string_in(tbl, "title_align");
    if (v.ok) {
        cfg->title_align = parse_text_align(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(tbl, "title_font");
    if (v.ok) {
        strncpy(cfg->title_style.family, v.u.s, sizeof(cfg->title_style.family) - 1);
        free(v.u.s);
    }
    
    v = toml_int_in(tbl, "title_font_size");
    if (v.ok) cfg->title_style.size = v.u.i;
    
    v = toml_string_in(tbl, "title_color");
    if (v.ok) {
        cfg->title_style.color = rgba_hex(parse_color(v.u.s));
        free(v.u.s);
    }
    
    v = toml_bool_in(tbl, "buttons_left");
    if (v.ok) cfg->buttons_left = v.u.b;
    
    v = toml_int_in(tbl, "button_spacing");
    if (v.ok) cfg->button_spacing = v.u.i;
    
    /* Parse button configurations */
    toml_table_t *btn_close = toml_table_in(tbl, "button_close");
    if (btn_close) parse_button_style(btn_close, &cfg->btn_close);
    
    toml_table_t *btn_max = toml_table_in(tbl, "button_maximize");
    if (btn_max) parse_button_style(btn_max, &cfg->btn_maximize);
    
    toml_table_t *btn_min = toml_table_in(tbl, "button_minimize");
    if (btn_min) parse_button_style(btn_min, &cfg->btn_minimize);
}

/* Load visual configuration from TOML file */
int visual_config_load(visual_config_t *config, const char *path) {
    if (!config || !path) return -1;
    
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    char errbuf[256];
    toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);
    
    if (!root) {
        fprintf(stderr, "Visual config parse error: %s\n", errbuf);
        return -1;
    }
    
    /* Check for theme preset first */
    toml_datum_t theme = toml_string_in(root, "theme");
    if (theme.ok) {
        theme_preset_t preset = parse_theme_preset(theme.u.s);
        *config = visual_config_preset(preset);
        free(theme.u.s);
    }
    
    /* Parse decoration settings */
    toml_table_t *decoration = toml_table_in(root, "decoration");
    if (decoration) {
        toml_datum_t v;
        
        v = toml_bool_in(decoration, "enabled");
        if (v.ok) config->decoration.enabled = v.u.b;
        
        /* Parse titlebar */
        toml_table_t *titlebar = toml_table_in(decoration, "titlebar");
        if (titlebar) {
            parse_titlebar_config(titlebar, &config->decoration.titlebar);
        }
    }
    
    /* Parse animation settings */
    toml_table_t *animation = toml_table_in(root, "animation");
    if (animation) {
        toml_datum_t v;
        
        v = toml_bool_in(animation, "enabled");
        if (v.ok) config->animations_enabled = v.u.b;
        
        v = toml_int_in(animation, "duration_ms");
        if (v.ok) config->animation_duration_ms = v.u.i;
    }
    
    toml_free(root);
    return 0;
}

/* Save visual configuration to TOML file */
int visual_config_save(visual_config_t *config, const char *path) {
    if (!config || !path) return -1;
    
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;
    
    fprintf(fp, "# Visual configuration for starview\n\n");
    
    fprintf(fp, "[decoration]\n");
    fprintf(fp, "enabled = %s\n", config->decoration.enabled ? "true" : "false");
    
    fprintf(fp, "\n[decoration.titlebar]\n");
    fprintf(fp, "height = %d\n", config->decoration.titlebar.height);
    fprintf(fp, "padding_left = %d\n", config->decoration.titlebar.padding_left);
    fprintf(fp, "padding_right = %d\n", config->decoration.titlebar.padding_right);
    fprintf(fp, "corner_radius = %.1f\n", config->decoration.titlebar.corner_radius_tl);
    fprintf(fp, "bg_color = \"#%08x\"\n", rgba_to_hex(config->decoration.titlebar.bg_color));
    fprintf(fp, "bg_color_inactive = \"#%08x\"\n", rgba_to_hex(config->decoration.titlebar.bg_color_inactive));
    fprintf(fp, "title_align = \"%s\"\n", 
        config->decoration.titlebar.title_align == TEXT_ALIGN_LEFT ? "left" :
        config->decoration.titlebar.title_align == TEXT_ALIGN_CENTER ? "center" : "right");
    fprintf(fp, "title_font = \"%s\"\n", config->decoration.titlebar.title_style.family);
    fprintf(fp, "title_font_size = %.0f\n", config->decoration.titlebar.title_style.size);
    fprintf(fp, "title_color = \"#%08x\"\n", rgba_to_hex(config->decoration.titlebar.title_style.color));
    fprintf(fp, "buttons_left = %s\n", config->decoration.titlebar.buttons_left ? "true" : "false");
    fprintf(fp, "button_spacing = %d\n", config->decoration.titlebar.button_spacing);
    
    fprintf(fp, "\n[animation]\n");
    fprintf(fp, "enabled = %s\n", config->animations_enabled ? "true" : "false");
    fprintf(fp, "duration_ms = %d\n", config->animation_duration_ms);
    
    fclose(fp);
    return 0;
}
