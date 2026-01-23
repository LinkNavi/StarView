#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <xkbcommon/xkbcommon.h>
#include <wlr/types/wlr_keyboard.h>
#include "gesture.h"
#include <linux/input-event-codes.h>
#include "gesture_config.h"
struct config config = {
    .gaps_inner = 5,
    .gaps_outer = 10,
    .border_width = 2,
    .border_color_active = 0x89b4faff,
    .border_color_inactive = 0x45475aff,
    .focus_follows_mouse = true,
    .default_mode = MODE_TILING,
    .resize_step = 50,
    .move_step = 50,
    .master_ratio = 0.55f,
    .master_count = 1,
    .keybind_count = 0,
    .autostart_count = 0,
   .gesture_swipe_threshold = 0.3,
    .gesture_pinch_threshold = 0.15,
    .gesture_mouse_threshold = 50.0,
    .gesture_touchpad = NULL,
    .gesture_touchpad_count = 0,
    .gesture_mouse = NULL,
    .gesture_mouse_count = 0,
    .rule_count = 0,
    .decor = {
        .enabled = true,
        .height = 30,
        .button_size = 12,
        .button_spacing = 8,
        .corner_radius = 8,
        .bg_color = 0x1e1e2eff,
        .bg_color_inactive = 0x313244ff,
        .title_color = 0xcdd6f4ff,
        .title_color_inactive = 0x6c7086ff,
        .btn_close_color = 0xf38ba8ff,
        .btn_close_hover = 0xeba0acff,
        .btn_max_color = 0xa6e3a1ff,
        .btn_max_hover = 0x94e2d5ff,
        .btn_min_color = 0xf9e2afff,
        .btn_min_hover = 0xf5c2e7ff,
        .font = "sans",
        .font_size = 12,
        .buttons_left = false,
    },
    .anim = {
        .enabled = true,
        .duration_ms = 200,
        .window_open = ANIM_ZOOM,
        .window_close = ANIM_FADE,
        .window_move = ANIM_NONE,
        .window_resize = ANIM_NONE,
        .workspace_switch = ANIM_SLIDE,
        .curve = CURVE_EASE_OUT,
        .fade_min = 0.0f,
        .zoom_min = 0.8f,
    },
};

static char config_path[512] = {0};
static int include_depth = 0;

/*
 * ACTION STRING MAP
 */
static struct {
    const char *name;
    enum keybind_action action;
} action_map[] = {
    {"spawn",               ACTION_SPAWN},
    {"close",               ACTION_CLOSE},
    {"fullscreen",          ACTION_FULLSCREEN},
    {"toggle_floating",     ACTION_TOGGLE_FLOATING},
    {"focus_left",          ACTION_FOCUS_LEFT},
    {"focus_right",         ACTION_FOCUS_RIGHT},
    {"focus_up",            ACTION_FOCUS_UP},
    {"focus_down",          ACTION_FOCUS_DOWN},
    {"focus_next",          ACTION_FOCUS_NEXT},
    {"focus_prev",          ACTION_FOCUS_PREV},
    {"focus",               ACTION_NONE},
    {"move_left",           ACTION_MOVE_LEFT},
    {"move_right",          ACTION_MOVE_RIGHT},
    {"move_up",             ACTION_MOVE_UP},
    {"move_down",           ACTION_MOVE_DOWN},
    {"move",                ACTION_NONE},
    {"resize_grow_width",   ACTION_RESIZE_GROW_WIDTH},
    {"resize_shrink_width", ACTION_RESIZE_SHRINK_WIDTH},
    {"resize_grow_height",  ACTION_RESIZE_GROW_HEIGHT},
    {"resize_shrink_height",ACTION_RESIZE_SHRINK_HEIGHT},
    {"workspace",           ACTION_WORKSPACE},
    {"move_to_workspace",   ACTION_MOVE_TO_WORKSPACE},
    {"workspace_next",      ACTION_WORKSPACE_NEXT},
    {"workspace_prev",      ACTION_WORKSPACE_PREV},
    {"mode_tiling",         ACTION_MODE_TILING},
    {"mode_floating",       ACTION_MODE_FLOATING},
    {"toggle_mode",         ACTION_TOGGLE_MODE},
    {"inc_master_count",    ACTION_INC_MASTER_COUNT},
    {"dec_master_count",    ACTION_DEC_MASTER_COUNT},
    {"inc_master_ratio",    ACTION_INC_MASTER_RATIO},
    {"dec_master_ratio",    ACTION_DEC_MASTER_RATIO},
    {"reload_config",       ACTION_RELOAD_CONFIG},
    {"reload",              ACTION_RELOAD_CONFIG},
    {"exit",                ACTION_EXIT},
    {"quit",                ACTION_EXIT},
    {"minimize",            ACTION_MINIMIZE},
    {"maximize",            ACTION_MAXIMIZE},
    {"snap_left",           ACTION_SNAP_LEFT},
    {"snap_right",          ACTION_SNAP_RIGHT},
    {"center",              ACTION_CENTER_WINDOW},
    {"preselect_left",   ACTION_PRESELECT_LEFT},
    {"preselect_right",  ACTION_PRESELECT_RIGHT},
    {"preselect_up",     ACTION_PRESELECT_UP},
    {"preselect_down",   ACTION_PRESELECT_DOWN},
    {"swap_left",        ACTION_SWAP_LEFT},
    {"swap_right",       ACTION_SWAP_RIGHT},
    {"swap_up",          ACTION_SWAP_UP},
    {"swap_down",        ACTION_SWAP_DOWN},
    {NULL, ACTION_NONE}
};


static enum gesture_direction parse_gesture_direction(const char *str) {
    if (!str) return GESTURE_DIR_NONE;
    if (strcasecmp(str, "up") == 0) return GESTURE_DIR_UP;
    if (strcasecmp(str, "down") == 0) return GESTURE_DIR_DOWN;
    if (strcasecmp(str, "left") == 0) return GESTURE_DIR_LEFT;
    if (strcasecmp(str, "right") == 0) return GESTURE_DIR_RIGHT;
    if (strcasecmp(str, "in") == 0) return GESTURE_DIR_IN;
    if (strcasecmp(str, "out") == 0) return GESTURE_DIR_OUT;
    return GESTURE_DIR_NONE;
}

static enum gesture_action_type parse_gesture_action_type(const char *str) {
    if (!str) return GESTURE_ACTION_NONE;
    if (strcasecmp(str, "workspace_next") == 0) return GESTURE_ACTION_WORKSPACE_NEXT;
    if (strcasecmp(str, "workspace_prev") == 0) return GESTURE_ACTION_WORKSPACE_PREV;
    if (strcasecmp(str, "workspace") == 0) return GESTURE_ACTION_WORKSPACE;
    if (strcasecmp(str, "maximize") == 0) return GESTURE_ACTION_MAXIMIZE;
    if (strcasecmp(str, "minimize") == 0) return GESTURE_ACTION_MINIMIZE;
    if (strcasecmp(str, "fullscreen") == 0) return GESTURE_ACTION_FULLSCREEN;
    if (strcasecmp(str, "close") == 0) return GESTURE_ACTION_CLOSE;
    if (strcasecmp(str, "snap_left") == 0) return GESTURE_ACTION_SNAP_LEFT;
    if (strcasecmp(str, "snap_right") == 0) return GESTURE_ACTION_SNAP_RIGHT;
    if (strcasecmp(str, "toggle_floating") == 0) return GESTURE_ACTION_TOGGLE_FLOATING;
    if (strcasecmp(str, "toggle_mode") == 0) return GESTURE_ACTION_TOGGLE_MODE;
    if (strcasecmp(str, "spawn") == 0) return GESTURE_ACTION_SPAWN;
    if (strcasecmp(str, "exec") == 0) return GESTURE_ACTION_EXEC;
    return GESTURE_ACTION_NONE;
}

static uint32_t parse_mouse_button(const char *str) {
    if (!str) return 0;
    if (strcasecmp(str, "left") == 0) return BTN_LEFT;
    if (strcasecmp(str, "middle") == 0) return BTN_MIDDLE;
    if (strcasecmp(str, "right") == 0) return BTN_RIGHT;
    return 0;
}

static void parse_gesture_touchpad(toml_array_t *arr) {
    if (!arr) return;
    
    int len = toml_array_nelem(arr);
    config.gesture_touchpad = calloc(len, sizeof(struct gesture_touchpad));
    
    for (int i = 0; i < len; i++) {
        toml_table_t *g = toml_table_at(arr, i);
        if (!g) continue;
        
        struct gesture_touchpad *gt = &config.gesture_touchpad[config.gesture_touchpad_count];
        
        toml_datum_t v;
        
        v = toml_int_in(g, "fingers");
        if (v.ok) gt->fingers = v.u.i;
        
        v = toml_string_in(g, "direction");
        if (v.ok) {
            gt->direction = parse_gesture_direction(v.u.s);
            free(v.u.s);
        }
        
        v = toml_string_in(g, "action");
        if (v.ok) {
            const char *space = strchr(v.u.s, ' ');
            if (space) {
                char action_str[64];
                size_t len = space - v.u.s;
                if (len >= sizeof(action_str)) len = sizeof(action_str) - 1;
                strncpy(action_str, v.u.s, len);
                action_str[len] = '\0';
                gt->action.type = parse_gesture_action_type(action_str);
                strncpy(gt->action.arg, space + 1, sizeof(gt->action.arg) - 1);
            } else {
                gt->action.type = parse_gesture_action_type(v.u.s);
            }
            free(v.u.s);
        }
        
        if (gt->fingers > 0 && gt->direction != GESTURE_DIR_NONE && 
            gt->action.type != GESTURE_ACTION_NONE) {
            config.gesture_touchpad_count++;
        }
    }
}

static void parse_gesture_mouse(toml_array_t *arr) {
    if (!arr) return;
    
    int len = toml_array_nelem(arr);
    config.gesture_mouse = calloc(len, sizeof(struct gesture_mouse));
    
    for (int i = 0; i < len; i++) {
        toml_table_t *g = toml_table_at(arr, i);
        if (!g) continue;
        
        struct gesture_mouse *gm = &config.gesture_mouse[config.gesture_mouse_count];
        
        toml_datum_t v;
        
        v = toml_string_in(g, "button");
        if (v.ok) {
            gm->button = parse_mouse_button(v.u.s);
            free(v.u.s);
        }
        
        v = toml_string_in(g, "modifiers");
        if (v.ok) {
            if (strstr(v.u.s, "Alt")) gm->modifiers |= WLR_MODIFIER_ALT;
            if (strstr(v.u.s, "Super")) gm->modifiers |= WLR_MODIFIER_LOGO;
            if (strstr(v.u.s, "Ctrl")) gm->modifiers |= WLR_MODIFIER_CTRL;
            if (strstr(v.u.s, "Shift")) gm->modifiers |= WLR_MODIFIER_SHIFT;
            free(v.u.s);
        }
        
        v = toml_string_in(g, "direction");
        if (v.ok) {
            gm->direction = parse_gesture_direction(v.u.s);
            free(v.u.s);
        }
        
        v = toml_string_in(g, "action");
        if (v.ok) {
            const char *space = strchr(v.u.s, ' ');
            if (space) {
                char action_str[64];
                size_t len = space - v.u.s;
                if (len >= sizeof(action_str)) len = sizeof(action_str) - 1;
                strncpy(action_str, v.u.s, len);
                action_str[len] = '\0';
                gm->action.type = parse_gesture_action_type(action_str);
                strncpy(gm->action.arg, space + 1, sizeof(gm->action.arg) - 1);
            } else {
                gm->action.type = parse_gesture_action_type(v.u.s);
            }
            free(v.u.s);
        }
        
        if (gm->button > 0 && gm->direction != GESTURE_DIR_NONE && 
            gm->action.type != GESTURE_ACTION_NONE) {
            config.gesture_mouse_count++;
        }
    }
}

static enum keybind_action parse_action(const char *str, char *arg_out) {
    arg_out[0] = '\0';
    
    const char *space = strchr(str, ' ');
    
    char action_str[64];
    if (space) {
        size_t len = space - str;
        if (len >= sizeof(action_str)) len = sizeof(action_str) - 1;
        strncpy(action_str, str, len);
        action_str[len] = '\0';
        strncpy(arg_out, space + 1, 127);
        arg_out[127] = '\0';
    } else {
        strncpy(action_str, str, sizeof(action_str) - 1);
        action_str[sizeof(action_str) - 1] = '\0';
    }
    
    for (int i = 0; action_map[i].name; i++) {
        if (strcasecmp(action_str, action_map[i].name) == 0) {
            enum keybind_action action = action_map[i].action;
            
            if (action == ACTION_NONE && arg_out[0]) {
                if (strcasecmp(action_str, "focus") == 0) {
                    if (strcasecmp(arg_out, "left") == 0) return ACTION_FOCUS_LEFT;
                    if (strcasecmp(arg_out, "right") == 0) return ACTION_FOCUS_RIGHT;
                    if (strcasecmp(arg_out, "up") == 0) return ACTION_FOCUS_UP;
                    if (strcasecmp(arg_out, "down") == 0) return ACTION_FOCUS_DOWN;
                    if (strcasecmp(arg_out, "next") == 0) return ACTION_FOCUS_NEXT;
                    if (strcasecmp(arg_out, "prev") == 0) return ACTION_FOCUS_PREV;
                }
                if (strcasecmp(action_str, "move") == 0) {
                    if (strcasecmp(arg_out, "left") == 0) return ACTION_MOVE_LEFT;
                    if (strcasecmp(arg_out, "right") == 0) return ACTION_MOVE_RIGHT;
                    if (strcasecmp(arg_out, "up") == 0) return ACTION_MOVE_UP;
                    if (strcasecmp(arg_out, "down") == 0) return ACTION_MOVE_DOWN;
                }
            }
            
            return action;
        }
    }
    
    fprintf(stderr, "Unknown action: %s\n", action_str);
    return ACTION_NONE;
}

uint32_t parse_color(const char *str) {
    if (!str) return 0;
    if (str[0] == '#') str++;
    
    uint32_t color = 0;
    int len = strlen(str);
    
    if (len == 6) {
        color = (uint32_t)strtoul(str, NULL, 16);
        color = (color << 8) | 0xff;
    } else if (len == 8) {
        color = (uint32_t)strtoul(str, NULL, 16);
    }
    
    return color;
}

bool parse_keybind(const char *str, uint32_t *mods, uint32_t *keysym) {
    *mods = 0;
    *keysym = 0;
    
    char buf[128];
    strncpy(buf, str, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    
    char *saveptr;
    char *token = strtok_r(buf, "+", &saveptr);
    char *last_token = NULL;
    
    while (token) {
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        last_token = token;
        
        if (strcasecmp(token, "Super") == 0 || strcasecmp(token, "Mod4") == 0 || 
            strcasecmp(token, "Logo") == 0) {
            *mods |= WLR_MODIFIER_LOGO;
        } else if (strcasecmp(token, "Alt") == 0 || strcasecmp(token, "Mod1") == 0) {
            *mods |= WLR_MODIFIER_ALT;
        } else if (strcasecmp(token, "Ctrl") == 0 || strcasecmp(token, "Control") == 0) {
            *mods |= WLR_MODIFIER_CTRL;
        } else if (strcasecmp(token, "Shift") == 0) {
            *mods |= WLR_MODIFIER_SHIFT;
        }
        
        token = strtok_r(NULL, "+", &saveptr);
    }
    
    if (last_token) {
        *keysym = xkb_keysym_from_name(last_token, XKB_KEYSYM_CASE_INSENSITIVE);
        if (*keysym == XKB_KEY_NoSymbol) {
            fprintf(stderr, "Unknown key: %s\n", last_token);
            return false;
        }
        
        if (*mods & WLR_MODIFIER_SHIFT) {
            *keysym = xkb_keysym_to_upper(*keysym);
        }
    }
    
    return *keysym != 0;
}

static void parse_keybinds(toml_table_t *keybinds_table) {
    if (!keybinds_table) return;
    
    int i = 0;
    const char *key;
    while ((key = toml_key_in(keybinds_table, i++)) != NULL) {
        if (config.keybind_count >= MAX_KEYBINDS) break;
        
        toml_datum_t val = toml_string_in(keybinds_table, key);
        if (!val.ok) continue;
        
        struct keybind *kb = &config.keybinds[config.keybind_count];
        
        if (!parse_keybind(key, &kb->modifiers, &kb->keysym)) {
            free(val.u.s);
            continue;
        }
        
        kb->action = parse_action(val.u.s, kb->arg);
        
        if (kb->action == ACTION_NONE) {
            free(val.u.s);
            continue;
        }
        
        config.keybind_count++;
        free(val.u.s);
    }
}

static void parse_autostart(toml_array_t *arr) {
    if (!arr) return;
    
    int len = toml_array_nelem(arr);
    for (int i = 0; i < len && config.autostart_count < 32; i++) {
        toml_datum_t val = toml_string_at(arr, i);
        if (val.ok) {
            config.autostart[config.autostart_count++] = val.u.s;
        }
    }
}

static enum anim_type parse_anim_type(const char *str) {
    if (!str) return ANIM_NONE;
    if (strcasecmp(str, "none") == 0) return ANIM_NONE;
    if (strcasecmp(str, "fade") == 0) return ANIM_FADE;
    if (strcasecmp(str, "slide") == 0) return ANIM_SLIDE;
    if (strcasecmp(str, "zoom") == 0) return ANIM_ZOOM;
    if (strcasecmp(str, "slide_fade") == 0) return ANIM_SLIDE_FADE;
    return ANIM_NONE;
}

static enum anim_curve parse_anim_curve(const char *str) {
    if (!str) return CURVE_LINEAR;
    if (strcasecmp(str, "linear") == 0) return CURVE_LINEAR;
    if (strcasecmp(str, "ease_in") == 0) return CURVE_EASE_IN;
    if (strcasecmp(str, "ease_out") == 0) return CURVE_EASE_OUT;
    if (strcasecmp(str, "ease_in_out") == 0) return CURVE_EASE_IN_OUT;
    if (strcasecmp(str, "bounce") == 0) return CURVE_BOUNCE;
    return CURVE_LINEAR;
}
static uint32_t parse_color_hex(const char *hex) {
    if (!hex) return 0x1E1E2E; // default color
    
    // Skip '#' if present
    if (hex[0] == '#') hex++;
    
    // Parse as hex - this gives us RRGGBB
    uint32_t color = 0;
    sscanf(hex, "%x", &color);
    
    // The TOML has 6-digit hex (#RRGGBB) but we store as 8-digit (0xRRGGBBFF)
    // Shift left 8 bits and add full alpha
    return (color << 8) | 0xFF;
}
void parse_decoration(toml_table_t *decor) {
    if (!decor) return;
    
    toml_datum_t v;
    
    v = toml_bool_in(decor, "enabled");
    if (v.ok) config.decor.enabled = v.u.b;
    
    v = toml_int_in(decor, "height");
    if (v.ok) config.decor.height = v.u.i;
    
    v = toml_int_in(decor, "button_size");
    if (v.ok) config.decor.button_size = v.u.i;
    
    v = toml_int_in(decor, "button_spacing");
    if (v.ok) config.decor.button_spacing = v.u.i;
    
    v = toml_int_in(decor, "corner_radius");
    if (v.ok) config.decor.corner_radius = v.u.i;
    
    // Colors - parse hex strings to uint32_t
    v = toml_string_in(decor, "bg_color");
    if (v.ok) {
        config.decor.bg_color = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "bg_color_inactive");
    if (v.ok) {
        config.decor.bg_color_inactive = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "title_color");
    if (v.ok) {
        config.decor.title_color = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "title_color_inactive");
    if (v.ok) {
        config.decor.title_color_inactive = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    // Button colors
    v = toml_string_in(decor, "close_color");
    if (v.ok) {
        config.decor.btn_close_color = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "close_hover");
    if (v.ok) {
        config.decor.btn_close_hover = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "maximize_color");
    if (v.ok) {
        config.decor.btn_max_color = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "maximize_hover");
    if (v.ok) {
        config.decor.btn_max_hover = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "minimize_color");
    if (v.ok) {
        config.decor.btn_min_color = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    v = toml_string_in(decor, "minimize_hover");
    if (v.ok) {
        config.decor.btn_min_hover = parse_color_hex(v.u.s);
        free(v.u.s);
    }
    
    // Font
    v = toml_string_in(decor, "font");
    if (v.ok) {
        strncpy(config.decor.font, v.u.s, sizeof(config.decor.font) - 1);
        free(v.u.s);
    }
    
    v = toml_int_in(decor, "font_size");
    if (v.ok) config.decor.font_size = v.u.i;
    
    v = toml_bool_in(decor, "buttons_left");
    if (v.ok) config.decor.buttons_left = v.u.b;
}

static void parse_animation(toml_table_t *anim) {
    if (!anim) return;
    
    toml_datum_t v;
    
    v = toml_bool_in(anim, "enabled");
    if (v.ok) config.anim.enabled = v.u.b;
    
    v = toml_int_in(anim, "duration");
    if (v.ok) config.anim.duration_ms = v.u.i;
    
    v = toml_int_in(anim, "speed");  // alias
    if (v.ok) config.anim.duration_ms = v.u.i;
    
    v = toml_string_in(anim, "window_open");
    if (v.ok) { config.anim.window_open = parse_anim_type(v.u.s); free(v.u.s); }
    
    v = toml_string_in(anim, "window_close");
    if (v.ok) { config.anim.window_close = parse_anim_type(v.u.s); free(v.u.s); }
    
    v = toml_string_in(anim, "window_move");
    if (v.ok) { config.anim.window_move = parse_anim_type(v.u.s); free(v.u.s); }
    
    v = toml_string_in(anim, "window_resize");
    if (v.ok) { config.anim.window_resize = parse_anim_type(v.u.s); free(v.u.s); }
    
    v = toml_string_in(anim, "workspace_switch");
    if (v.ok) { config.anim.workspace_switch = parse_anim_type(v.u.s); free(v.u.s); }
    
    v = toml_string_in(anim, "curve");
    if (v.ok) { config.anim.curve = parse_anim_curve(v.u.s); free(v.u.s); }
    
    v = toml_double_in(anim, "fade_min");
    if (v.ok) config.anim.fade_min = v.u.d;
    
    v = toml_double_in(anim, "zoom_min");
    if (v.ok) config.anim.zoom_min = v.u.d;
}

static void parse_window_rules(toml_array_t *rules) {
    if (!rules) return;
    
    int len = toml_array_nelem(rules);
    for (int i = 0; i < len && config.rule_count < MAX_WINDOW_RULES; i++) {
        toml_table_t *rule = toml_table_at(rules, i);
        if (!rule) continue;
        
        struct window_rule *r = &config.rules[config.rule_count];
        memset(r, 0, sizeof(*r));
        r->opacity = 1.0f;
        r->workspace = -1;
        
        toml_datum_t v;
        
        v = toml_string_in(rule, "app_id");
        if (v.ok) { strncpy(r->app_id, v.u.s, sizeof(r->app_id) - 1); free(v.u.s); }
        
        v = toml_string_in(rule, "title");
        if (v.ok) { strncpy(r->title, v.u.s, sizeof(r->title) - 1); free(v.u.s); }
        
        v = toml_bool_in(rule, "floating");
        if (v.ok) r->floating = v.u.b;
        
        v = toml_bool_in(rule, "fullscreen");
        if (v.ok) r->fullscreen = v.u.b;
        
        v = toml_int_in(rule, "workspace");
        if (v.ok) r->workspace = v.u.i;
        
        v = toml_int_in(rule, "x");
        if (v.ok) { r->x = v.u.i; r->has_position = true; }
        
        v = toml_int_in(rule, "y");
        if (v.ok) { r->y = v.u.i; r->has_position = true; }
        
        v = toml_int_in(rule, "width");
        if (v.ok) { r->width = v.u.i; r->has_size = true; }
        
        v = toml_int_in(rule, "height");
        if (v.ok) { r->height = v.u.i; r->has_size = true; }
        
        v = toml_double_in(rule, "opacity");
        if (v.ok) { r->opacity = v.u.d; r->has_opacity = true; }
        
        config.rule_count++;
    }
}

static int load_config_file(const char *path);

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static void load_config_dir(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;
    
    // Collect .toml files
    char *files[128];
    int file_count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < 128) {
        if (entry->d_name[0] == '.') continue;
        
        size_t len = strlen(entry->d_name);
        if (len < 5 || strcmp(entry->d_name + len - 5, ".toml") != 0) continue;
        
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);
        
        struct stat st;
        if (stat(fullpath, &st) == 0 && S_ISREG(st.st_mode)) {
            files[file_count++] = strdup(fullpath);
        }
    }
    closedir(dir);
    
    // Sort alphabetically for deterministic load order
    qsort(files, file_count, sizeof(char *), compare_strings);
    
    // Load each file
    for (int i = 0; i < file_count; i++) {
        load_config_file(files[i]);
        free(files[i]);
    }
}

static int load_config_file(const char *path) {
    if (include_depth >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Config include depth exceeded\n");
        return -1;
    }
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open config: %s\n", path);
        return -1;
    }
    
    printf("Loading config: %s\n", path);
    include_depth++;
    
    char errbuf[256];
    toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);
    
    if (!root) {
        fprintf(stderr, "Config parse error in %s: %s\n", path, errbuf);
        include_depth--;
        return -1;
    }
    
    // Handle includes first
    toml_array_t *includes = toml_array_in(root, "include");
    if (includes) {
        int len = toml_array_nelem(includes);
        for (int i = 0; i < len; i++) {
            toml_datum_t inc = toml_string_at(includes, i);
            if (!inc.ok) continue;
            
            char fullpath[512];
            if (inc.u.s[0] == '/') {
                strncpy(fullpath, inc.u.s, sizeof(fullpath) - 1);
            } else {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", config.config_dir, inc.u.s);
            }
            
            struct stat st;
            if (stat(fullpath, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    load_config_dir(fullpath);
                } else {
                    load_config_file(fullpath);
                }
            }
            free(inc.u.s);
        }
    }
     // [gestures]
    toml_table_t *gestures = toml_table_in(root, "gestures");
    if (gestures) {
        toml_datum_t v;
        
        v = toml_double_in(gestures, "swipe_threshold");
        if (v.ok) config.gesture_swipe_threshold = v.u.d;
        
        v = toml_double_in(gestures, "pinch_threshold");
        if (v.ok) config.gesture_pinch_threshold = v.u.d;
        
        v = toml_double_in(gestures, "mouse_threshold");
        if (v.ok) config.gesture_mouse_threshold = v.u.d;
    }
    
    // [[gesture_touchpad]]
    parse_gesture_touchpad(toml_array_in(root, "gesture_touchpad"));
    
    // [[gesture_mouse]]
    parse_gesture_mouse(toml_array_in(root, "gesture_mouse"));
    // [general]
    toml_table_t *general = toml_table_in(root, "general");
    if (general) {
        toml_datum_t v;
        
        v = toml_int_in(general, "gaps_inner");
        if (v.ok) config.gaps_inner = v.u.i;
        
        v = toml_int_in(general, "gaps_outer");
        if (v.ok) config.gaps_outer = v.u.i;
        
        v = toml_int_in(general, "border_width");
        if (v.ok) config.border_width = v.u.i;
        
        v = toml_string_in(general, "border_color_active");
        if (v.ok) { config.border_color_active = parse_color(v.u.s); free(v.u.s); }
        
        v = toml_string_in(general, "border_color_inactive");
        if (v.ok) { config.border_color_inactive = parse_color(v.u.s); free(v.u.s); }
        
        v = toml_bool_in(general, "focus_follows_mouse");
        if (v.ok) config.focus_follows_mouse = v.u.b;
        
        v = toml_string_in(general, "default_mode");
        if (v.ok) {
            if (strcasecmp(v.u.s, "floating") == 0) {
                config.default_mode = MODE_FLOATING;
            } else {
                config.default_mode = MODE_TILING;
            }
            free(v.u.s);
        }
        
        v = toml_int_in(general, "resize_step");
        if (v.ok) config.resize_step = v.u.i;
        
        v = toml_int_in(general, "move_step");
        if (v.ok) config.move_step = v.u.i;
    }
    
    // [decoration]
    parse_decoration(toml_table_in(root, "decoration"));
    
    // [animation]
    parse_animation(toml_table_in(root, "animation"));
    
    // [tiling]
    toml_table_t *tiling = toml_table_in(root, "tiling");
    if (tiling) {
        toml_datum_t v;
        
        v = toml_double_in(tiling, "master_ratio");
        if (v.ok) config.master_ratio = v.u.d;
        
        v = toml_int_in(tiling, "master_count");
        if (v.ok) config.master_count = v.u.i;
    }
    
    // [keybinds]
    parse_keybinds(toml_table_in(root, "keybinds"));
    
    // [[rules]]
    parse_window_rules(toml_array_in(root, "rules"));
    
    // autostart
    parse_autostart(toml_array_in(root, "autostart"));
    
    toml_free(root);
    include_depth--;
    
    return 0;
}

int config_load(const char *path) {
    strncpy(config_path, path, sizeof(config_path) - 1);
    
    // Extract config directory
    char *last_slash = strrchr(config_path, '/');
    if (last_slash) {
        size_t len = last_slash - config_path;
        strncpy(config.config_dir, config_path, len);
        config.config_dir[len] = '\0';
    } else {
        strcpy(config.config_dir, ".");
    }
    
    int ret = load_config_file(path);
    
    printf("Config loaded: %d keybinds, %d rules, %d autostart\n", 
           config.keybind_count, config.rule_count, config.autostart_count);
    
    return ret;
}

int config_reload(void) {
    if (config_path[0] == '\0') return -1;
    
 free(config.gesture_touchpad);
    config.gesture_touchpad = NULL;
    config.gesture_touchpad_count = 0;
    
    free(config.gesture_mouse);
    config.gesture_mouse = NULL;
    config.gesture_mouse_count = 0;

    // Clear autostart
    for (int i = 0; i < config.autostart_count; i++) {
        free(config.autostart[i]);
        config.autostart[i] = NULL;
    }
    config.autostart_count = 0;
    config.keybind_count = 0;
    config.rule_count = 0;
    
    printf("Reloading config...\n");
    return config_load(config_path);
}
