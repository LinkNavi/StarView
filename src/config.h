#ifndef CONFIG_H
#define CONFIG_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdbool.h>

#define MAX_KEYBINDS 128
#define MAX_WORKSPACES 10
#define MAX_WINDOW_RULES 64
#define MAX_INCLUDE_DEPTH 8

/*
 * WINDOW MODES
 */
enum window_mode {
    MODE_TILING,
    MODE_FLOATING,
};

/*
 * ANIMATION TYPES
 */
enum anim_type {
    ANIM_NONE = 0,
    ANIM_FADE,
    ANIM_SLIDE,
    ANIM_ZOOM,
    ANIM_SLIDE_FADE,
    ANIM_SLIDE_UP,
    ANIM_SLIDE_DOWN,
    ANIM_SLIDE_LEFT,
    ANIM_SLIDE_RIGHT,
};

enum anim_curve {
    CURVE_LINEAR = 0,
    CURVE_EASE_IN,
    CURVE_EASE_OUT,
    CURVE_EASE_IN_OUT,
    CURVE_BOUNCE,
    CURVE_SPRING,
};

/*
 * DECORATION BUTTON TYPES
 */
enum decor_button {
    BTN_CLOSE = 0,
    BTN_MAXIMIZE,
    BTN_MINIMIZE,
    BTN_COUNT,
};

/*
 * KEYBIND ACTIONS
 */
enum keybind_action {
    ACTION_NONE = 0,
    ACTION_SPAWN,
    ACTION_CLOSE,
    ACTION_FULLSCREEN,
    ACTION_TOGGLE_FLOATING,
    ACTION_FOCUS_LEFT,
    ACTION_FOCUS_RIGHT,
    ACTION_FOCUS_UP,
    ACTION_FOCUS_DOWN,
    ACTION_FOCUS_NEXT,
    ACTION_FOCUS_PREV,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_MOVE_UP,
    ACTION_MOVE_DOWN,
    ACTION_RESIZE_GROW_WIDTH,
    ACTION_RESIZE_SHRINK_WIDTH,
    ACTION_RESIZE_GROW_HEIGHT,
    ACTION_RESIZE_SHRINK_HEIGHT,
    ACTION_WORKSPACE,
    ACTION_MOVE_TO_WORKSPACE,
    ACTION_WORKSPACE_NEXT,
    ACTION_WORKSPACE_PREV,
    ACTION_MODE_TILING,
    ACTION_MODE_FLOATING,
    ACTION_TOGGLE_MODE,
    ACTION_INC_MASTER_COUNT,
    ACTION_DEC_MASTER_COUNT,
    ACTION_INC_MASTER_RATIO,
    ACTION_DEC_MASTER_RATIO,
    ACTION_RELOAD_CONFIG,
    ACTION_EXIT,
    ACTION_MINIMIZE,
    ACTION_MAXIMIZE,
    ACTION_SNAP_LEFT,
    ACTION_SNAP_RIGHT,
    ACTION_CENTER_WINDOW,
};

struct keybind {
    uint32_t modifiers;
    uint32_t keysym;
    enum keybind_action action;
    char arg[128];
};

/*
 * WINDOW RULES
 */
struct window_rule {
    char app_id[64];
    char title[128];
    bool floating;
    bool fullscreen;
    int workspace;
    int x, y, width, height;
    float opacity;
    bool has_position;
    bool has_size;
    bool has_opacity;
};

/*
 * ANIMATION CONFIG
 */
struct anim_config {
    bool enabled;
    int duration_ms;
    enum anim_type window_open;
    enum anim_type window_close;
    enum anim_type window_move;
    enum anim_type window_resize;
    enum anim_type workspace_switch;
    enum anim_curve curve;
    float fade_min;
    float zoom_min;
};

/*
 * DECORATION CONFIG
 */
struct decor_config {
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
    bool buttons_left;  // macOS style if true
};

/*
 * CONFIG STRUCT
 */
struct config {
    // [general]
    int gaps_inner;
    int gaps_outer;
    int border_width;
    uint32_t border_color_active;
    uint32_t border_color_inactive;
    bool focus_follows_mouse;
    enum window_mode default_mode;
    
    // [decoration]
    struct decor_config decor;
    
    // [animation]
    struct anim_config anim;
    
    // [tiling]
    float master_ratio;
    int master_count;
    
    // [resize]
    int resize_step;
    int move_step;
    
    // Keybinds
    struct keybind keybinds[MAX_KEYBINDS];
    int keybind_count;
    
    // Window rules
    struct window_rule rules[MAX_WINDOW_RULES];
    int rule_count;
    
    // Startup
    char *autostart[32];
    int autostart_count;
    
    // Include paths
    char config_dir[256];
};

extern struct config config;

int config_load(const char *path);
int config_reload(void);
uint32_t parse_color(const char *str);
bool parse_keybind(const char *str, uint32_t *mods, uint32_t *keysym);

#endif
