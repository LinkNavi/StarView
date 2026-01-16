#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <xkbcommon/xkbcommon.h>
#include <wlr/types/wlr_keyboard.h>

struct config config = {
    .gaps_inner = 5,
    .gaps_outer = 10,
    .border_width = 2,
    .border_color_active = 0x89b4faff,
    .border_color_inactive = 0x45475aff,
    .focus_follows_mouse = true,
    .default_mode = MODE_TILING,
    .animations_enabled = true,
    .animation_speed = 200,
    .master_ratio = 0.55f,
    .master_count = 1,
    .keybind_count = 0,
    .autostart_count = 0,
};

static char config_path[512] = {0};

/*
 * ACTION STRING MAP
 */
static struct {
    const char *name;
    enum keybind_action action;
} action_map[] = {
    // Spawning
    {"spawn",               ACTION_SPAWN},
    
    // Window management
    {"close",               ACTION_CLOSE},
    {"fullscreen",          ACTION_FULLSCREEN},
    {"toggle_floating",     ACTION_TOGGLE_FLOATING},
    
    // Focus
    {"focus_left",          ACTION_FOCUS_LEFT},
    {"focus_right",         ACTION_FOCUS_RIGHT},
    {"focus_up",            ACTION_FOCUS_UP},
    {"focus_down",          ACTION_FOCUS_DOWN},
    {"focus_next",          ACTION_FOCUS_NEXT},
    {"focus_prev",          ACTION_FOCUS_PREV},
    {"focus",               ACTION_NONE},
    
    // Move windows
    {"move_left",           ACTION_MOVE_LEFT},
    {"move_right",          ACTION_MOVE_RIGHT},
    {"move_up",             ACTION_MOVE_UP},
    {"move_down",           ACTION_MOVE_DOWN},
    {"move",                ACTION_NONE},
    
    // Resize
    {"resize_grow_width",   ACTION_RESIZE_GROW_WIDTH},
    {"resize_shrink_width", ACTION_RESIZE_SHRINK_WIDTH},
    {"resize_grow_height",  ACTION_RESIZE_GROW_HEIGHT},
    {"resize_shrink_height",ACTION_RESIZE_SHRINK_HEIGHT},
    
    // Workspaces
    {"workspace",           ACTION_WORKSPACE},
    {"move_to_workspace",   ACTION_MOVE_TO_WORKSPACE},
    {"workspace_next",      ACTION_WORKSPACE_NEXT},
    {"workspace_prev",      ACTION_WORKSPACE_PREV},
    
    // Mode switching
    {"mode_tiling",         ACTION_MODE_TILING},
    {"mode_floating",       ACTION_MODE_FLOATING},
    {"toggle_mode",         ACTION_TOGGLE_MODE},
    
    // Master-stack
    {"inc_master_count",    ACTION_INC_MASTER_COUNT},
    {"dec_master_count",    ACTION_DEC_MASTER_COUNT},
    {"inc_master_ratio",    ACTION_INC_MASTER_RATIO},
    {"dec_master_ratio",    ACTION_DEC_MASTER_RATIO},
    
    // Compositor
    {"reload_config",       ACTION_RELOAD_CONFIG},
    {"reload",              ACTION_RELOAD_CONFIG},
    {"exit",                ACTION_EXIT},
    {"quit",                ACTION_EXIT},
    
    {NULL, ACTION_NONE}
};

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
    if (!keybinds_table) {
        printf("DEBUG: No keybinds table found!\n");
        return;
    }
    
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
        
        printf("Keybind: '%s' -> action=%d arg='%s' (mods=0x%x sym=0x%x)\n",
               key, kb->action, kb->arg, kb->modifiers, kb->keysym);
        
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

int config_load(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Config not found: %s (using defaults)\n", path);
        return -1;
    }
    
    strncpy(config_path, path, sizeof(config_path) - 1);
    
    char errbuf[256];
    toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);
    
    if (!root) {
        fprintf(stderr, "Config parse error: %s\n", errbuf);
        return -1;
    }
    
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
    }
    
    // [animation]
    toml_table_t *animation = toml_table_in(root, "animation");
    if (animation) {
        toml_datum_t v;
        
        v = toml_bool_in(animation, "enabled");
        if (v.ok) config.animations_enabled = v.u.b;
        
        v = toml_int_in(animation, "speed");
        if (v.ok) config.animation_speed = v.u.i;
    }
    
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
    config.keybind_count = 0;
    toml_table_t *keybinds = toml_table_in(root, "keybinds");
    parse_keybinds(keybinds);
    
    // autostart
    toml_array_t *autostart = toml_array_in(root, "autostart");
    parse_autostart(autostart);
    
    toml_free(root);
    
    printf("Config loaded: %d keybinds, %d autostart, mode=%s\n", 
           config.keybind_count, config.autostart_count,
           config.default_mode == MODE_TILING ? "tiling" : "floating");
    
    return 0;
}

int config_reload(void) {
    if (config_path[0] == '\0') return -1;
    
    for (int i = 0; i < config.autostart_count; i++) {
        free(config.autostart[i]);
        config.autostart[i] = NULL;
    }
    config.autostart_count = 0;
    
    printf("Reloading config...\n");
    return config_load(config_path);
}
