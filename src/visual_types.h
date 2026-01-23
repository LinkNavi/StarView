/* visual_types.h - Type definitions for visual system */
#ifndef VISUAL_TYPES_H
#define VISUAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Theme preset enumeration */
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
} theme_preset_t;

/* Forward declarations */
typedef struct titlebar_config_t titlebar_config_t;

/* Color structure */
typedef struct {
    float r, g, b, a;
} rgba_t;

/* Border style */
typedef struct {
    int width;
    rgba_t color;
    int radius;
} border_t;

/* Shadow style */
typedef struct {
    bool enabled;
    int radius;
    int offset_x;
    int offset_y;
    rgba_t color;
} shadow_t;

/* Gradient types */
typedef enum {
    GRAD_NONE,
    GRAD_LINEAR_V,
    GRAD_LINEAR_H,
    GRAD_RADIAL,
} gradient_type_t;

typedef struct {
    gradient_type_t type;
    rgba_t start;
    rgba_t end;
} gradient_t;

/* Text style */
typedef enum {
    FONT_WEIGHT_NORMAL = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_BOLD = 700,
} font_weight_t;

typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT,
} text_align_t;

typedef struct {
    char font_family[64];
    int font_size;
    font_weight_t font_weight;
    rgba_t color;
} text_style_t;

/* Button types */
typedef enum {
    ICON_CLOSE,
    ICON_MAXIMIZE,
    ICON_MINIMIZE,
} icon_type_t;

/* Button icon structure */
typedef struct {
    rgba_t color;
    float scale;
} button_icon_t;

/* Button appearance for each state */
typedef struct {
    rgba_t bg_color;
    button_icon_t icon;
} button_appearance_t;

typedef struct {
    int width;
    int height;
    button_appearance_t states[4]; // Normal, Hover, Pressed, Disabled
} button_style_t;

/* Complete titlebar configuration */
struct titlebar_config_t {
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
    shadow_t shadow;
    shadow_t inner_shadow;
    
    int corner_radius_tl;
    int corner_radius_tr;
    int corner_radius_bl;
    int corner_radius_br;
    
    text_style_t title_style;
    text_style_t title_style_inactive;
    text_align_t title_align;
    int title_max_width;
    
    bool buttons_visible;
    bool buttons_left;
    int button_spacing;
    int button_margin;
    
    button_style_t btn_close;
    button_style_t btn_maximize;
    button_style_t btn_minimize;
    
    int button_order[4]; // Array of button indices, -1 terminated
    
    float inactive_opacity;
    bool separator_visible;
    int transition_ms;
};

/* Subpixel rendering order */
typedef enum {
    SUBPIXEL_NONE,
    SUBPIXEL_RGB,
    SUBPIXEL_BGR,
    SUBPIXEL_VRGB,
    SUBPIXEL_VBGR,
} subpixel_order_t;

/* Font hinting mode */
typedef enum {
    HINT_NONE,
    HINT_SLIGHT,
    HINT_MEDIUM,
    HINT_FULL,
} hint_mode_t;

/* Visual effects configuration */
struct effects_config_t {
    bool focus_ring_enabled;
    uint32_t focus_ring_color;
    int focus_ring_width;
    
    bool glow_enabled;
    uint32_t glow_color;
    int glow_radius;
    float glow_intensity;
    
    bool backdrop_blur_enabled;
    int backdrop_blur_radius;
    
    bool vibrancy_enabled;
    float vibrancy_amount;
};

typedef struct effects_config_t effects_config_t;

/* Workspace indicator configuration */
struct workspace_indicator_config_t {
    bool enabled;
    int position_x;
    int position_y;
    int indicator_size;
    int indicator_spacing;
    uint32_t bg_color;
    uint32_t active_color;
    uint32_t inactive_color;
    uint32_t urgent_color;
    int corner_radius;
};

typedef struct workspace_indicator_config_t workspace_indicator_config_t;

/* Main visual configuration */
typedef struct {
    /* Decoration settings */
    struct {
        bool enabled;
        struct titlebar_config_t titlebar;
        
        /* Window borders and shadows */
        border_t window_border;
        border_t window_border_inactive;
        border_t window_border_focused;
        
        shadow_t window_shadow;
        shadow_t window_shadow_inactive;
        shadow_t window_shadow_maximized;
        
        int resize_handle_size;
        
rgba_t resize_handle_color;
        bool resize_handle_visible;
    } decoration;
    
    /* Visual effects */
    effects_config_t effects;
    
    /* Workspace indicator */
    workspace_indicator_config_t workspace_indicator;
    
    /* Animation settings */
    bool animations_enabled;
    int animation_duration_ms;
    float animation_curve;
    
    /* Font rendering */
    bool font_antialias;
    bool font_subpixel;
    subpixel_order_t subpixel_order;
    hint_mode_t font_hinting;
    
    /* Display settings */
    float scale_factor;
    int dpi;
} visual_config_t;

#endif /* VISUAL_TYPES_H */
