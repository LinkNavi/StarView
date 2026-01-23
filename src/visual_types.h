/* visual_types.h - Type definitions for visual system */
#ifndef VISUAL_TYPES_H
#define VISUAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations to avoid circular dependencies */
struct titlebar_config_t;
struct decoration_config_t;
struct effects_config_t;
struct workspace_indicator_config_t;

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
typedef struct {
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
} effects_config_t;

/* Workspace indicator configuration */
typedef struct {
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
} workspace_indicator_config_t;

/* Main visual configuration */
typedef struct {
    /* Decoration settings - using existing config.h types */
    struct {
        bool enabled;
        struct titlebar_config_t titlebar;  /* Will be defined in config.h */
        
        /* Window borders and shadows */
        int border_width;
        uint32_t border_color;
        uint32_t border_color_inactive;
        uint32_t border_color_focused;
        
        int shadow_enabled;
        int shadow_radius;
        uint32_t shadow_color;
        
        int resize_handle_size;
        uint32_t resize_handle_color;
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
