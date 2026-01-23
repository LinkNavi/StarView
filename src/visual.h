#ifndef VISUAL_H
#define VISUAL_H

#include <stdint.h>
#include <stdbool.h>
#include <cairo/cairo.h>

/* ============================================================================
 * COLOR SYSTEM - RGBA with helpers
 * ============================================================================ */

typedef struct {
    float r, g, b, a;
} rgba_t;

rgba_t rgba_hex(uint32_t hex);
rgba_t rgba_rgb(float r, float g, float b);
rgba_t rgba_rgba(float r, float g, float b, float a);
rgba_t rgba_hsl(float h, float s, float l);
rgba_t rgba_hsla(float h, float s, float l, float a);
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
    GRAD_LINEAR_V,      /* Top to bottom */
    GRAD_LINEAR_H,      /* Left to right */
    GRAD_LINEAR_DIAG,   /* Top-left to bottom-right */
    GRAD_LINEAR_DIAG2,  /* Top-right to bottom-left */
    GRAD_RADIAL,        /* Center outward */
    GRAD_RADIAL_CORNER, /* Corner outward */
    GRAD_CONIC,         /* Angular sweep */
} gradient_type_t;

typedef struct {
    float position;     /* 0.0 - 1.0 */
    rgba_t color;
} gradient_stop_t;

typedef struct {
    gradient_type_t type;
    gradient_stop_t stops[8];
    int stop_count;
    float angle;        /* For linear gradients, override direction */
    float cx, cy;       /* Center for radial (0-1 range) */
    float radius;       /* For radial */
} gradient_t;

gradient_t gradient_simple(gradient_type_t type, rgba_t start, rgba_t end);
gradient_t gradient_three(gradient_type_t type, rgba_t a, rgba_t b, rgba_t c);
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
void cairo_draw_shadow_box(cairo_t *cr, double x, double y, double w, double h,
                           double *radii, shadow_t *shadow);

/* ============================================================================
 * BORDER SYSTEM
 * ============================================================================ */

typedef enum {
    BORDER_SOLID = 0,
    BORDER_DASHED,
    BORDER_DOTTED,
    BORDER_DOUBLE,
    BORDER_GROOVE,
    BORDER_RIDGE,
    BORDER_INSET,
    BORDER_OUTSET,
    BORDER_NONE,
} border_style_t;

typedef struct {
    float width;
    rgba_t color;
    border_style_t style;
    /* Per-corner radii: top-left, top-right, bottom-right, bottom-left */
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

typedef enum {
    SHAPE_RECT,
    SHAPE_ROUNDED_RECT,
    SHAPE_CIRCLE,
    SHAPE_ELLIPSE,
    SHAPE_PILL,
    SHAPE_SQUIRCLE,     /* iOS-style superellipse */
    SHAPE_DIAMOND,
    SHAPE_HEXAGON,
    SHAPE_OCTAGON,
    SHAPE_CUSTOM,       /* Custom path */
} shape_type_t;

/* Path a shape for filling/stroking */
void cairo_path_rect(cairo_t *cr, double x, double y, double w, double h);
void cairo_path_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r);
void cairo_path_rounded_rect_corners(cairo_t *cr, double x, double y, double w, double h,
                                     double tl, double tr, double br, double bl);
void cairo_path_circle(cairo_t *cr, double cx, double cy, double r);
void cairo_path_ellipse(cairo_t *cr, double cx, double cy, double rx, double ry);
void cairo_path_pill(cairo_t *cr, double x, double y, double w, double h);
void cairo_path_squircle(cairo_t *cr, double x, double y, double w, double h, double n);
void cairo_path_diamond(cairo_t *cr, double x, double y, double w, double h);
void cairo_path_hexagon(cairo_t *cr, double cx, double cy, double r);
void cairo_path_octagon(cairo_t *cr, double cx, double cy, double r);

/* ============================================================================
 * ICON SYSTEM
 * ============================================================================ */

typedef enum {
    ICON_NONE = 0,
    /* Window controls */
    ICON_CLOSE,
    ICON_MAXIMIZE,
    ICON_MINIMIZE,
    ICON_RESTORE,
    ICON_SHADE,
    ICON_PIN,
    ICON_MENU,
    /* Arrows */
    ICON_ARROW_UP,
    ICON_ARROW_DOWN,
    ICON_ARROW_LEFT,
    ICON_ARROW_RIGHT,
    ICON_CHEVRON_UP,
    ICON_CHEVRON_DOWN,
    ICON_CHEVRON_LEFT,
    ICON_CHEVRON_RIGHT,
    /* Misc */
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
    ICON_STYLE_LINE,        /* Outlined icons */
    ICON_STYLE_FILLED,      /* Solid icons */
    ICON_STYLE_DUOTONE,     /* Two-tone icons */
} icon_style_t;

typedef struct {
    icon_type_t type;
    icon_style_t style;
    rgba_t color;
    rgba_t secondary_color;  /* For duotone */
    float size;
    float stroke_width;
    float padding;
    bool antialiased;
} icon_t;

icon_t icon_create(icon_type_t type, float size, rgba_t color);
void cairo_draw_icon(cairo_t *cr, double x, double y, icon_t *icon);

/* ============================================================================
 * BUTTON STYLE
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
    float scale;            /* Transform scale on press */
    float opacity;
} button_style_state_t;

typedef struct {
    shape_type_t shape;
    float width;
    float height;
    float squircle_n;       /* For squircle shape */
    button_style_state_t states[BTN_STATE_COUNT];
    float transition_ms;    /* Animation duration */
} button_style_t;

button_style_t button_style_circle(float size, rgba_t color, icon_type_t icon);
button_style_t button_style_rect(float w, float h, rgba_t color, icon_type_t icon);
button_style_t button_style_pill(float w, float h, rgba_t color, icon_type_t icon);

/* ============================================================================
 * TITLEBAR CONFIGURATION
 * ============================================================================ */

typedef struct {
    /* Dimensions */
    int height;
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
    
    /* Background */
    rgba_t bg_color;
    rgba_t bg_color_inactive;
    gradient_t bg_gradient;
    gradient_t bg_gradient_inactive;
    
    /* Borders */
    border_t border;
    border_t border_inactive;
    
    /* Shadows */
    shadow_t shadow;
    shadow_t inner_shadow;
    
    /* Corner radius (for top corners only usually) */
    float corner_radius_tl;
    float corner_radius_tr;
    float corner_radius_br;
    float corner_radius_bl;
    
    /* Title text */
    text_style_t title_style;
    text_style_t title_style_inactive;
    text_align_t title_align;
    int title_offset_x;     /* Manual adjustment */
    int title_offset_y;
    int title_max_width;    /* 0 = auto */
    
    /* Buttons */
    bool buttons_visible;
    bool buttons_left;      /* macOS style */
    int button_spacing;
    int button_margin;
    button_style_t btn_close;
    button_style_t btn_maximize;
    button_style_t btn_minimize;
    
    /* Button order (indices: 0=close, 1=max, 2=min, -1=end) */
    int button_order[4];
    
    /* Extra effects */
    float blur_radius;      /* Background blur (if compositor supports) */
    float inactive_opacity;
    bool separator_visible;
    rgba_t separator_color;
    int separator_height;
    
    /* Animation */
    int transition_ms;
} titlebar_config_t;

/* ============================================================================
 * WINDOW DECORATION CONFIGURATION
 * ============================================================================ */

typedef struct {
    bool enabled;
    
    /* Border around entire window */
    border_t window_border;
    border_t window_border_inactive;
    border_t window_border_focused;  /* When keyboard focused */
    
    /* Window shadow */
    shadow_t window_shadow;
    shadow_t window_shadow_inactive;
    shadow_t window_shadow_maximized;
    
    /* Resize handles */
    int resize_handle_size;
    rgba_t resize_handle_color;
    bool resize_handle_visible;
    
    /* Titlebar */
    titlebar_config_t titlebar;
    
    /* Client area styling */
    rgba_t client_bg_color;     /* Behind client if not opaque */
    float client_corner_radius;
    
    /* Effects */
    float inactive_desaturate;  /* Desaturate inactive windows */
    float inactive_dim;         /* Dim inactive windows */
} decoration_config_t;

/* ============================================================================
 * VISUAL EFFECTS
 * ============================================================================ */

typedef struct {
    /* Focus indicator */
    bool focus_ring_enabled;
    rgba_t focus_ring_color;
    float focus_ring_width;
    float focus_ring_offset;
    
    /* Window glow */
    bool glow_enabled;
    rgba_t glow_color;
    float glow_radius;
    float glow_intensity;
    
    /* Backdrop blur (placeholder - needs compositor support) */
    bool backdrop_blur_enabled;
    float backdrop_blur_radius;
    
    /* Vibrancy / Transparency */
    bool vibrancy_enabled;
    rgba_t vibrancy_tint;
    float vibrancy_amount;
} effects_config_t;

/* ============================================================================
 * WORKSPACE INDICATOR CONFIGURATION
 * ============================================================================ */

typedef struct {
    bool enabled;
    
    /* Positioning */
    enum { WS_POS_TOP, WS_POS_BOTTOM, WS_POS_LEFT, WS_POS_RIGHT } position;
    int offset_x;
    int offset_y;
    int margin;
    
    /* Container style */
    rgba_t bg_color;
    gradient_t bg_gradient;
    border_t border;
    shadow_t shadow;
    float corner_radius;
    int padding;
    
    /* Indicator style */
    enum { WS_STYLE_DOT, WS_STYLE_NUMBER, WS_STYLE_PILL, WS_STYLE_SQUARE } style;
    int indicator_size;
    int indicator_spacing;
    
    /* Colors */
    rgba_t active_color;
    rgba_t inactive_color;
    rgba_t urgent_color;
    rgba_t empty_color;
    rgba_t hover_color;
    
    /* Text (for number style) */
    text_style_t number_style;
    
    /* Animation */
    int transition_ms;
    bool animate_switch;
} workspace_indicator_config_t;

/* ============================================================================
 * COMPLETE VISUAL CONFIGURATION
 * ============================================================================ */

typedef struct {
    decoration_config_t decoration;
    effects_config_t effects;
    workspace_indicator_config_t workspace_indicator;
    
    /* Global settings */
    bool animations_enabled;
    int animation_duration_ms;
    float animation_curve;      /* Bezier control point */
    
    /* Font rendering */
    bool font_antialias;
    bool font_subpixel;
    enum { SUBPIXEL_RGB, SUBPIXEL_BGR, SUBPIXEL_VRGB, SUBPIXEL_VBGR } subpixel_order;
    enum { HINT_NONE, HINT_SLIGHT, HINT_MEDIUM, HINT_FULL } font_hinting;
    
    /* DPI/Scaling */
    float scale_factor;
    int dpi;
} visual_config_t;

/* ============================================================================
 * PRESET THEMES
 * ============================================================================ */

typedef enum {
    THEME_DEFAULT,
    THEME_MACOS_LIGHT,
    THEME_MACOS_DARK,
    THEME_WINDOWS_11,
    THEME_GNOME_ADWAITA,
    THEME_GNOME_ADWAITA_DARK,
    THEME_KDE_BREEZE,
    THEME_KDE_BREEZE_DARK,
    THEME_MINIMAL,
    THEME_MINIMAL_DARK,
    THEME_CATPPUCCIN_MOCHA,
    THEME_CATPPUCCIN_LATTE,
    THEME_NORD,
    THEME_DRACULA,
    THEME_GRUVBOX_DARK,
    THEME_SOLARIZED_DARK,
    THEME_TOKYO_NIGHT,
    THEME_ONE_DARK,
    THEME_CUSTOM,
} theme_preset_t;

/* Load a preset theme */
visual_config_t visual_config_preset(theme_preset_t preset);

/* Create default config */
visual_config_t visual_config_default(void);

/* Load from TOML file */
int visual_config_load(visual_config_t *config, const char *path);

/* Save to TOML file */
int visual_config_save(visual_config_t *config, const char *path);

/* ============================================================================
 * RENDERING CONTEXT
 * ============================================================================ */

struct wlr_scene_tree;
struct wlr_scene_buffer;
struct wlr_renderer;

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

/* Draw complete titlebar */
void draw_titlebar(render_surface_t *rs, titlebar_config_t *config,
                   const char *title, bool active, int hover_button);

/* Draw window frame/border */
void draw_window_frame(render_surface_t *rs, decoration_config_t *config,
                       int width, int height, bool active);

/* Draw button with current state */
void draw_button(cairo_t *cr, double x, double y, button_style_t *style,
                 button_state_t state);

/* Draw text with style */
void draw_text(cairo_t *cr, double x, double y, double max_width,
               const char *text, text_style_t *style);

/* Draw workspace indicator */
void draw_workspace_indicator(render_surface_t *rs, workspace_indicator_config_t *config,
                              int current, int total, int *urgent, int urgent_count);

#endif /* VISUAL_H */
