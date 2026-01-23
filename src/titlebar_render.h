#ifndef TITLEBAR_RENDER_H
#define TITLEBAR_RENDER_H

#include <stdint.h>
#include <stdbool.h>
#include <wayland-server-core.h>

struct wlr_scene_tree;
struct wlr_scene_buffer;
struct toplevel;
struct decor_config;
struct shadow_config;  // Forward declaration
/*
 * Button types
 */
enum button_type {
    BTN_TYPE_CLOSE,
    BTN_TYPE_MAXIMIZE,
    BTN_TYPE_MINIMIZE,
    BTN_TYPE_CUSTOM,
};

/*
 * Button states for styling
 */
enum button_state {
    BTN_STATE_NORMAL,
    BTN_STATE_HOVER,
    BTN_STATE_PRESSED,
    BTN_STATE_DISABLED,
};

/*
 * Element alignment
 */
enum element_align {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
};

/*
 * Shape types for buttons/elements
 */
enum shape_type {
    SHAPE_RECT,
    SHAPE_ROUNDED_RECT,
    SHAPE_CIRCLE,
    SHAPE_PILL,
    SHAPE_CUSTOM,  /* Use SVG path */
};

/*
 * Gradient direction
 */
enum gradient_dir {
    GRADIENT_NONE,
    GRADIENT_VERTICAL,
    GRADIENT_HORIZONTAL,
    GRADIENT_DIAGONAL,
};

/*
 * Color with alpha
 */
struct color {
    float r, g, b, a;
};

/*
 * Gradient definition
 */
struct gradient {
    enum gradient_dir direction;
    struct color start;
    struct color end;
};

/*
 * Border definition
 */
struct border_style {
    float width;
    struct color color;
    float radius;  /* Corner radius */
};

/*
 * Shadow definition
 */
struct shadow_style {
    bool enabled;
    float offset_x;
    float offset_y;
    float blur;
    struct color color;
};

/*
 * Text style
 */
struct text_style {
    char font_family[64];
    int font_size;
    int font_weight;  /* 400 = normal, 700 = bold */
    bool italic;
    struct color color;
    struct shadow_style shadow;
};

/*
 * Button style for each state
 */
struct button_style {
    struct color bg_color;
    struct gradient bg_gradient;
    struct border_style border;
    struct shadow_style shadow;
    
    /* Icon styling */
    struct color icon_color;
    float icon_scale;
    
    /* SVG icon path (optional) */
    char icon_path[256];
    
    /* Built-in icon character (for symbol fonts) */
    char icon_char[8];
};

/*
 * Complete button theme (all states)
 */
struct button_theme {
    enum button_type type;
    enum shape_type shape;
    int width;
    int height;
    int margin;
    
    struct button_style normal;
    struct button_style hover;
    struct button_style pressed;
    struct button_style disabled;
};

/*
 * Titlebar theme
 */
struct titlebar_theme {
    /* Dimensions */
    int height;
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
    
    /* Background */
    struct color bg_color;
    struct gradient bg_gradient;
    struct border_style border;
    struct shadow_style shadow;
    float corner_radius_top;
    float corner_radius_bottom;
    
    /* Title text */
    struct text_style title;
    struct text_style title_inactive;
    enum element_align title_align;
    int title_max_width;  /* 0 = auto */
    
    /* Button layout */
    bool buttons_left;  /* macOS style */
    int button_spacing;
    int button_margin;  /* Distance from edge */
    
    /* Individual button themes */
    struct button_theme btn_close;
    struct button_theme btn_maximize;
    struct button_theme btn_minimize;
    
    /* Extra buttons (custom) */
    struct button_theme *extra_buttons;
    int extra_button_count;
    
    /* Active/Inactive variations */
    struct color bg_color_inactive;
    struct gradient bg_gradient_inactive;
    float inactive_opacity;
};

/*
 * Rendered titlebar state
 */
struct rendered_titlebar {
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *background;
    struct wlr_scene_buffer *btn_close;
    struct wlr_scene_buffer *btn_maximize;
    struct wlr_scene_buffer *btn_minimize;
    struct wlr_scene_buffer *title;
    
    /* Button hit boxes */
    struct {
        int x, y, width, height;
    } close_box, max_box, min_box;
    
    /* Current state */
    int width;
    int height;
    bool active;
    enum button_state close_state;
    enum button_state max_state;
    enum button_state min_state;
    
    /* Cached title */
    char cached_title[256];
};

/*
 * Theme presets
 */
enum theme_preset {
    THEME_PRESET_DEFAULT,
    THEME_PRESET_MACOS,
    THEME_PRESET_WINDOWS,
    THEME_PRESET_GNOME,
    THEME_PRESET_MINIMAL,
    THEME_PRESET_FLAT,
    THEME_PRESET_CUSTOM,
};

/* ============================================================================
 * API Functions
 * ============================================================================ */

/* Theme management */
struct titlebar_theme *titlebar_theme_create(void);
void titlebar_theme_destroy(struct titlebar_theme *theme);
void titlebar_theme_load_preset(struct titlebar_theme *theme, enum theme_preset preset);
int titlebar_theme_load_file(struct titlebar_theme *theme, const char *path);
int titlebar_theme_save_file(struct titlebar_theme *theme, const char *path);
// In titlebar_render.h
int titlebar_theme_load_from_config(struct titlebar_theme *theme, 
                                     struct decor_config *config);
struct wlr_scene_buffer *create_shadow_buffer(
    struct wlr_scene_tree *parent,
    int window_width,
    int window_height,
    struct shadow_config *shadow);
/* Color helpers */
struct color color_from_hex(uint32_t hex);
struct color color_from_rgba(float r, float g, float b, float a);
uint32_t color_to_hex(struct color c);
struct color color_blend(struct color a, struct color b, float t);
struct color color_lighten(struct color c, float amount);
struct color color_darken(struct color c, float amount);

/* Titlebar rendering */
struct rendered_titlebar *titlebar_render_create(struct wlr_scene_tree *parent,
                                                  struct titlebar_theme *theme);
void titlebar_render_destroy(struct rendered_titlebar *tb);
void titlebar_render_update(struct rendered_titlebar *tb, 
                            struct titlebar_theme *theme,
                            int width, 
                            const char *title, 
                            bool active);
void titlebar_render_set_button_state(struct rendered_titlebar *tb,
                                       enum button_type btn,
                                       enum button_state state);

/* Hit testing */
enum button_type titlebar_render_hit_test(struct rendered_titlebar *tb, 
                                           int x, int y);
bool titlebar_render_in_drag_area(struct rendered_titlebar *tb, int x, int y);

/* Global theme */
void titlebar_set_global_theme(struct titlebar_theme *theme);
struct titlebar_theme *titlebar_get_global_theme(void);

#endif /* TITLEBAR_RENDER_H */
