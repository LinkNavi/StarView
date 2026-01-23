#ifndef VISUAL_H
#define VISUAL_H

#include <stdint.h>
#include <stdbool.h>
#include <cairo/cairo.h>

/* ============================================================================
 * SHAPE TYPES - Define before use
 * ============================================================================ */

typedef enum {
    SHAPE_RECT,
    SHAPE_ROUNDED_RECT,
    SHAPE_CIRCLE,
    SHAPE_ELLIPSE,
    SHAPE_PILL,
    SHAPE_SQUIRCLE,
    SHAPE_DIAMOND,
    SHAPE_HEXAGON,
    SHAPE_OCTAGON,
    SHAPE_CUSTOM,
} shape_type_t;

/* ============================================================================
 * COLOR SYSTEM
 * ============================================================================ */

typedef struct {
    float r, g, b, a;
} rgba_t;

rgba_t rgba_hex(uint32_t hex);
rgba_t rgba_rgb(float r, float g, float b);
rgba_t rgba_rgba(float r, float g, float b, float a);
rgba_t rgba_blend(rgba_t a, rgba_t b, float t);
rgba_t rgba_lighten(rgba_t c, float amount);
rgba_t rgba_darken(rgba_t c, float amount);
rgba_t rgba_saturate(rgba_t c, float amount);
rgba_t rgba_desaturate(rgba_t c, float amount);
rgba_t rgba_alpha(rgba_t c, float a);
uint32_t rgba_to_hex(rgba_t c);
void cairo_set_rgba(cairo_t *cr, rgba_t c);

/* ============================================================================
 * GRADIENT SYSTEM
 * ============================================================================ */

typedef enum {
    GRAD_NONE = 0,
    GRAD_LINEAR_V,
    GRAD_LINEAR_H,
    GRAD_LINEAR_DIAG,
    GRAD_LINEAR_DIAG2,
    GRAD_RADIAL,
    GRAD_RADIAL_CORNER,
    GRAD_CONIC,
} gradient_type_t;

typedef struct {
    float position;
    rgba_t color;
} gradient_stop_t;

typedef struct {
    gradient_type_t type;
    gradient_stop_t stops[8];
    int stop_count;
    float angle;
    float cx, cy;
    float radius;
} gradient_t;

gradient_t gradient_simple(gradient_type_t type, rgba_t start, rgba_t end);
void gradient_add_stop(gradient_t *g, float pos, rgba_t color);
cairo_pattern_t *gradient_to_pattern(gradient_t *g, double x, double y, double w, double h);

/* ============================================================================
 * SHADOW SYSTEM
 * ============================================================================ */

typedef struct {
    bool enabled;
    float offset_x;
    float offset_y;
    float blur;
    float spread;
    rgba_t color;
    bool inset;
} shadow_t;

shadow_t shadow_none(void);
shadow_t shadow_drop(float blur, rgba_t color);
shadow_t shadow_soft(float blur, float spread, rgba_t color);
shadow_t shadow_hard(float ox, float oy, rgba_t color);
shadow_t shadow_glow(float blur, rgba_t color);
shadow_t shadow_inset(float blur, rgba_t color);

void cairo_draw_shadow(cairo_t *cr, double x, double y, double w, double h,
                       double radius, shadow_t *shadow);

/* ============================================================================
 * BORDER SYSTEM
 * ============================================================================ */

typedef enum {
    BORDER_SOLID = 0,
    BORDER_DASHED,
    BORDER_DOTTED,
    BORDER_DOUBLE,
    BORDER_NONE,
} border_style_t;

typedef struct {
    float width;
    rgba_t color;
    border_style_t style;
    float radius[4];
} border_t;

border_t border_none(void);
border_t border_solid(float width, rgba_t color, float radius);
border_t border_rounded(float width, rgba_t color, float tl, float tr, float br, float bl);

void cairo_draw_border(cairo_t *cr, double x, double y, double w, double h, border_t *border);

/* ============================================================================
 * TEXT STYLE SYSTEM
 * ============================================================================ */

typedef enum {
    FONT_WEIGHT_THIN = 100,
    FONT_WEIGHT_EXTRA_LIGHT = 200,
    FONT_WEIGHT_LIGHT = 300,
    FONT_WEIGHT_NORMAL = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_SEMI_BOLD = 600,
    FONT_WEIGHT_BOLD = 700,
    FONT_WEIGHT_EXTRA_BOLD = 800,
    FONT_WEIGHT_BLACK = 900,
} font_weight_t;

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT,
} text_align_t;

typedef enum {
    TEXT_OVERFLOW_CLIP,
    TEXT_OVERFLOW_ELLIPSIS,
    TEXT_OVERFLOW_FADE,
} text_overflow_t;

typedef struct {
    char family[64];
    float size;
    font_weight_t weight;
    bool italic;
    rgba_t color;
    shadow_t shadow;
    float letter_spacing;
    float line_height;
    text_align_t align;
    text_overflow_t overflow;
    bool antialias;
    bool subpixel;
} text_style_t;

text_style_t text_style_default(void);
text_style_t text_style_create(const char *family, float size, font_weight_t weight, rgba_t color);

/* ============================================================================
 * SHAPE DRAWING
 * ============================================================================ */

void cairo_path_rect(cairo_t *cr, double x, double y, double w, double h);
void cairo_path_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);
void cairo_path_rounded_rect_corners(cairo_t *cr, double x, double y, double w, double h,
                                     double tl, double tr, double br, double bl);
void cairo_path_circle(cairo_t *cr, double cx, double cy, double r);
void cairo_path_pill(cairo_t *cr, double x, double y, double w, double h);
void cairo_path_squircle(cairo_t *cr, double x, double y, double w, double h, double n);

/* ============================================================================
 * ICON SYSTEM
 * ============================================================================ */

typedef enum {
    ICON_NONE = 0,
    ICON_CLOSE,
    ICON_MAXIMIZE,
    ICON_MINIMIZE,
    ICON_RESTORE,
    ICON_SHADE,
    ICON_PIN,
    ICON_MENU,
    ICON_ARROW_UP,
    ICON_ARROW_DOWN,
    ICON_ARROW_LEFT,
    ICON_ARROW_RIGHT,
    ICON_CHEVRON_UP,
    ICON_CHEVRON_DOWN,
    ICON_CHEVRON_LEFT,
    ICON_CHEVRON_RIGHT,
    ICON_PLUS,
    ICON_MINUS,
    ICON_CHECK,
    ICON_X,
    ICON_DOT,
    ICON_RING,
    ICON_HAMBURGER,
    ICON_GRID,
    ICON_CUSTOM,
} icon_type_t;

typedef enum {
    ICON_STYLE_LINE,
    ICON_STYLE_FILLED,
    ICON_STYLE_DUOTONE,
} icon_style_t;

typedef struct {
    icon_type_t type;
    icon_style_t style;
    rgba_t color;
    rgba_t secondary_color;
    float size;
    float stroke_width;
    float padding;
    bool antialiased;
} icon_t;

icon_t icon_create(icon_type_t type, float size, rgba_t color);
void cairo_draw_icon(cairo_t *cr, double x, double y, icon_t *icon);

/* ============================================================================
 * BUTTON STYLE - Use same enum as titlebar_render.h
 * ============================================================================ */

typedef enum {
    BTN_STATE_NORMAL = 0,
    BTN_STATE_HOVER,
    BTN_STATE_PRESSED,
    BTN_STATE_DISABLED,
    BTN_STATE_FOCUSED,
    BTN_STATE_COUNT,
} button_state_t;

typedef struct {
    rgba_t bg_color;
    gradient_t bg_gradient;
    border_t border;
    shadow_t shadow;
    icon_t icon;
    float scale;
    float opacity;
} button_style_state_t;

typedef struct {
    shape_type_t shape;
    float width;
    float height;
    float squircle_n;
    button_style_state_t states[BTN_STATE_COUNT];
    float transition_ms;
} button_style_t;

button_style_t button_style_circle(float size, rgba_t color, icon_type_t icon);
button_style_t button_style_rect(float w, float h, rgba_t color, icon_type_t icon);
button_style_t button_style_pill(float w, float h, rgba_t color, icon_type_t icon);

/* ============================================================================
 * TITLEBAR CONFIGURATION
 * ============================================================================ */

typedef struct {
    int height;
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
    
    rgba_t bg_color;
    rgba_t bg_color_inactive;
    gradient_t bg_gradient;
    gradient_t bg_gradient_inactive;
    
    border_t border;
    border_t border_inactive;
    
    shadow_t shadow;
    shadow_t inner_shadow;
    
    float corner_radius_tl;
    float corner_radius_tr;
    float corner_radius_br;
    float corner_radius_bl;
    
    text_style_t title_style;
    text_style_t title_style_inactive;
    text_align_t title_align;
    int title_offset_x;
    int title_offset_y;
    int title_max_width;
    
    bool buttons_visible;
    bool buttons_left;
    int button_spacing;
    int button_margin;
    button_style_t btn_close;
    button_style_t btn_maximize;
    button_style_t btn_minimize;
    
    int button_order[4];
    
    float blur_radius;
    float inactive_opacity;
    bool separator_visible;
    rgba_t separator_color;
    int separator_height;
    
    int transition_ms;
} titlebar_config_t;

/* ============================================================================
 * THEME PRESETS
 * ============================================================================ */

typedef enum {
    THEME_DEFAULT,
    THEME_MACOS_LIGHT,
    THEME_MACOS_DARK,
    THEME_WINDOWS_11,
    THEME_GNOME_ADWAITA,
    THEME_GNOME_ADWAITA_DARK,
    THEME_MINIMAL,
    THEME_MINIMAL_DARK,
    THEME_CATPPUCCIN_MOCHA,
    THEME_CATPPUCCIN_LATTE,
    THEME_NORD,
    THEME_DRACULA,
    THEME_GRUVBOX_DARK,
    THEME_TOKYO_NIGHT,
    THEME_ONE_DARK,
    THEME_CUSTOM,
} theme_preset_t;

/* Create titlebar config from preset */
titlebar_config_t titlebar_config_preset(theme_preset_t preset);

/* Create default titlebar config */
titlebar_config_t titlebar_config_default(void);

/* ============================================================================
 * RENDERING CONTEXT
 * ============================================================================ */

struct wlr_scene_tree;
struct wlr_scene_buffer;

typedef struct {
    struct wlr_scene_tree *tree;
    struct wlr_scene_buffer *buffer;
    cairo_surface_t *surface;
    cairo_t *cr;
    int width;
    int height;
    bool dirty;
} render_surface_t;

render_surface_t *render_surface_create(struct wlr_scene_tree *parent, int width, int height);
void render_surface_resize(render_surface_t *rs, int width, int height);
void render_surface_commit(render_surface_t *rs);
void render_surface_destroy(render_surface_t *rs);

/* ============================================================================
 * HIGH-LEVEL DRAWING
 * ============================================================================ */

void draw_titlebar(render_surface_t *rs, titlebar_config_t *config,
                   const char *title, bool active, int hover_button);

void draw_button(cairo_t *cr, double x, double y, button_style_t *style,
                 button_state_t state);

void draw_text(cairo_t *cr, double x, double y, double max_width,
               const char *text, text_style_t *style);

#endif /* VISUAL_H */
