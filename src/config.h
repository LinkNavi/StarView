#ifndef CONFIG_H
#define CONFIG_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdbool.h>

#define MAX_KEYBINDS 128
#define MAX_WORKSPACES 10

/*
 * ============================================================================
 * WINDOW MODES
 * ============================================================================
 */
enum window_mode {
    MODE_TILING,    // Hyprland-style auto-tiling
    MODE_FLOATING,  // KDE-style free windows
};

/*
 * ============================================================================
 * KEYBIND ACTIONS
 * ============================================================================
 */
enum keybind_action {
    ACTION_NONE = 0,
    
    // Spawning
    ACTION_SPAWN,
    
    // Window management
    ACTION_CLOSE,
    ACTION_FULLSCREEN,
    ACTION_TOGGLE_FLOATING,
    
    // Focus
    ACTION_FOCUS_LEFT,
    ACTION_FOCUS_RIGHT,
    ACTION_FOCUS_UP,
    ACTION_FOCUS_DOWN,
    ACTION_FOCUS_NEXT,
    ACTION_FOCUS_PREV,
    
    // Move windows
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_MOVE_UP,
    ACTION_MOVE_DOWN,
    
    // Resize
    ACTION_RESIZE_GROW_WIDTH,
    ACTION_RESIZE_SHRINK_WIDTH,
    ACTION_RESIZE_GROW_HEIGHT,
    ACTION_RESIZE_SHRINK_HEIGHT,
    
    // Workspaces
    ACTION_WORKSPACE,
    ACTION_MOVE_TO_WORKSPACE,
    ACTION_WORKSPACE_NEXT,
    ACTION_WORKSPACE_PREV,
    
    // Mode switching
    ACTION_MODE_TILING,
    ACTION_MODE_FLOATING,
    ACTION_TOGGLE_MODE,
    
    // Master-stack adjustments
    ACTION_INC_MASTER_COUNT,
    ACTION_DEC_MASTER_COUNT,
    ACTION_INC_MASTER_RATIO,
    ACTION_DEC_MASTER_RATIO,
    
    // Compositor
    ACTION_RELOAD_CONFIG,
    ACTION_EXIT,
};

struct keybind {
    uint32_t modifiers;
    uint32_t keysym;
    enum keybind_action action;
    char arg[128];
};

/*
 * ============================================================================
 * CONFIG STRUCT
 * ============================================================================
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
    
    // [animation]
    bool animations_enabled;
    int animation_speed;
    
    // [tiling]
    float master_ratio;
    int master_count;
    
    // Keybinds
    struct keybind keybinds[MAX_KEYBINDS];
    int keybind_count;
    
    // Startup
    char *autostart[32];
    int autostart_count;
};

extern struct config config;

int config_load(const char *path);
int config_reload(void);
uint32_t parse_color(const char *str);
bool parse_keybind(const char *str, uint32_t *mods, uint32_t *keysym);

#endif
