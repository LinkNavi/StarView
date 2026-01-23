#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <linux/input-event-codes.h>

enum gesture_type {
    GESTURE_NONE,
    GESTURE_SWIPE,
    GESTURE_PINCH,
    GESTURE_HOLD,
};

struct gesture_state {
    bool active;
    enum gesture_type type;
    uint32_t fingers;
    
    // Swipe
    double dx, dy;
    
    // Pinch
    double scale;
    double rotation;
    
    // Mouse gestures
    bool mouse_active;
    uint32_t mouse_button;
    uint32_t mouse_mods;
    double mouse_start_x, mouse_start_y;
    double mouse_dx, mouse_dy;
};

struct gesture_state gesture_state = {0};

static bool check_modifiers(uint32_t mods, uint32_t required) {
    if (required == 0) return mods == 0;
    return (mods & required) == required;
}

static void execute_gesture_action(struct server *server, struct gesture_action *action) {
    if (!action) return;
    
    switch (action->type) {
    case GESTURE_ACTION_WORKSPACE_NEXT: {
        int ws = server->current_workspace + 1;
        if (ws > MAX_WORKSPACES) ws = 1;
        workspace_show(server, ws);
        break;
    }
    
    case GESTURE_ACTION_WORKSPACE_PREV: {
        int ws = server->current_workspace - 1;
        if (ws < 1) ws = MAX_WORKSPACES;
        workspace_show(server, ws);
        break;
    }
    
    case GESTURE_ACTION_WORKSPACE: {
        int ws = atoi(action->arg);
        if (ws >= 1 && ws <= MAX_WORKSPACES) {
            workspace_show(server, ws);
        }
        break;
    }
    
    case GESTURE_ACTION_MAXIMIZE: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused || focused->maximized) break;
        
        focused->maximized = true;
        struct wlr_scene_node *node = focused->decor.tree ?
            &focused->decor.tree->node : &focused->scene_tree->node;
        focused->pre_max_x = node->x;
        focused->pre_max_y = node->y;
        struct wlr_box geo;
        wlr_xdg_surface_get_geometry(focused->xdg_toplevel->base, &geo);
        focused->pre_max_width = geo.width;
        focused->pre_max_height = geo.height;
        
        struct output *output;
        wl_list_for_each(output, &server->outputs, link) {
            int w = output->wlr_output->width - config.gaps_outer * 2;
            int h = output->wlr_output->height - config.gaps_outer * 2;
            if (config.decor.enabled) h -= config.decor.height;
            
            wlr_scene_node_set_position(node, config.gaps_outer, config.gaps_outer);
            wlr_xdg_toplevel_set_size(focused->xdg_toplevel, w, h);
            break;
        }
        break;
    }
    
    case GESTURE_ACTION_MINIMIZE: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused) break;
        
        focused->minimized = true;
        struct wlr_scene_node *node = focused->decor.tree ?
            &focused->decor.tree->node : &focused->scene_tree->node;
        wlr_scene_node_set_enabled(node, false);
        break;
    }
    
    case GESTURE_ACTION_FULLSCREEN: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused) break;
        
        focused->fullscreen = !focused->fullscreen;
        wlr_xdg_toplevel_set_fullscreen(focused->xdg_toplevel, focused->fullscreen);
        break;
    }
    
    case GESTURE_ACTION_CLOSE: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (focused) {
            wlr_xdg_toplevel_send_close(focused->xdg_toplevel);
        }
        break;
    }
    
    case GESTURE_ACTION_SNAP_LEFT: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused) break;
        
        struct output *output;
        wl_list_for_each(output, &server->outputs, link) {
            int w = (output->wlr_output->width - config.gaps_outer * 3) / 2;
            int h = output->wlr_output->height - config.gaps_outer * 2;
            if (config.decor.enabled) h -= config.decor.height;
            
            struct wlr_scene_node *node = focused->decor.tree ?
                &focused->decor.tree->node : &focused->scene_tree->node;
            wlr_scene_node_set_position(node, config.gaps_outer, config.gaps_outer);
            wlr_xdg_toplevel_set_size(focused->xdg_toplevel, w, h);
            break;
        }
        break;
    }
    
    case GESTURE_ACTION_SNAP_RIGHT: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused) break;
        
        struct output *output;
        wl_list_for_each(output, &server->outputs, link) {
            int w = (output->wlr_output->width - config.gaps_outer * 3) / 2;
            int h = output->wlr_output->height - config.gaps_outer * 2;
            int x = output->wlr_output->width / 2 + config.gaps_outer / 2;
            if (config.decor.enabled) h -= config.decor.height;
            
            struct wlr_scene_node *node = focused->decor.tree ?
                &focused->decor.tree->node : &focused->scene_tree->node;
            wlr_scene_node_set_position(node, x, config.gaps_outer);
            wlr_xdg_toplevel_set_size(focused->xdg_toplevel, w, h);
            break;
        }
        break;
    }
    
    case GESTURE_ACTION_TOGGLE_FLOATING: {
        struct toplevel *focused = get_focused_toplevel(server);
        if (!focused) break;
        
        focused->floating = !focused->floating;
        arrange_windows(server);
        break;
    }
    
    case GESTURE_ACTION_TOGGLE_MODE: {
        if (server->mode == MODE_TILING) {
            server->mode = MODE_FLOATING;
        } else {
            server->mode = MODE_TILING;
        }
        arrange_windows(server);
        break;
    }
    
    case GESTURE_ACTION_SPAWN: {
        if (action->arg[0]) {
            if (fork() == 0) {
                execl("/bin/sh", "sh", "-c", action->arg, NULL);
                _exit(1);
            }
        }
        break;
    }
    
    case GESTURE_ACTION_EXEC: {
        if (action->arg[0]) {
            if (fork() == 0) {
                execl("/bin/sh", "sh", "-c", action->arg, NULL);
                _exit(1);
            }
        }
        break;
    }
    
    default:
        break;
    }
}

static struct gesture_action *find_touchpad_action(int fingers, 
                                                    enum gesture_direction dir) {
    for (int i = 0; i < config.gesture_touchpad_count; i++) {
        struct gesture_touchpad *g = &config.gesture_touchpad[i];
        if (g->fingers == fingers && g->direction == dir) {
            return &g->action;
        }
    }
    return NULL;
}

void gesture_swipe_begin(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_swipe_begin);
    struct wlr_pointer_swipe_begin_event *event = data;
    
    gesture_state.active = true;
    gesture_state.type = GESTURE_SWIPE;
    gesture_state.fingers = event->fingers;
    gesture_state.dx = 0;
    gesture_state.dy = 0;
}

void gesture_swipe_update(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_swipe_update);
    struct wlr_pointer_swipe_update_event *event = data;
    
    if (!gesture_state.active || gesture_state.type != GESTURE_SWIPE) return;
    
    gesture_state.dx += event->dx;
    gesture_state.dy += event->dy;
}

void gesture_swipe_end(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_swipe_end);
    struct wlr_pointer_swipe_end_event *event = data;
    
    if (!gesture_state.active || gesture_state.type != GESTURE_SWIPE) return;
    
    if (event->cancelled) {
        gesture_state.active = false;
        return;
    }
    
    // Determine direction
    enum gesture_direction dir = GESTURE_DIR_NONE;
    
    if (fabs(gesture_state.dx) > fabs(gesture_state.dy)) {
        if (gesture_state.dx > config.gesture_swipe_threshold) {
            dir = GESTURE_DIR_RIGHT;
        } else if (gesture_state.dx < -config.gesture_swipe_threshold) {
            dir = GESTURE_DIR_LEFT;
        }
    } else {
        if (gesture_state.dy > config.gesture_swipe_threshold) {
            dir = GESTURE_DIR_DOWN;
        } else if (gesture_state.dy < -config.gesture_swipe_threshold) {
            dir = GESTURE_DIR_UP;
        }
    }
    
    if (dir != GESTURE_DIR_NONE) {
        struct gesture_action *action = find_touchpad_action(gesture_state.fingers, dir);
        if (action) {
            execute_gesture_action(server, action);
        }
    }
    
    gesture_state.active = false;
}

void gesture_pinch_begin(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_pinch_begin);
    struct wlr_pointer_pinch_begin_event *event = data;
    
    gesture_state.active = true;
    gesture_state.type = GESTURE_PINCH;
    gesture_state.fingers = event->fingers;
    gesture_state.scale = 1.0;
    gesture_state.rotation = 0;
}

void gesture_pinch_update(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_pinch_update);
    struct wlr_pointer_pinch_update_event *event = data;
    
    if (!gesture_state.active || gesture_state.type != GESTURE_PINCH) return;
    
    gesture_state.scale = event->scale;
    gesture_state.rotation = event->rotation;
}

void gesture_pinch_end(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_pinch_end);
    struct wlr_pointer_pinch_end_event *event = data;
    
    if (!gesture_state.active || gesture_state.type != GESTURE_PINCH) return;
    
    if (event->cancelled) {
        gesture_state.active = false;
        return;
    }
    
    enum gesture_direction dir = GESTURE_DIR_NONE;
    
    if (gesture_state.scale > 1.0 + config.gesture_pinch_threshold) {
        dir = GESTURE_DIR_OUT;
    } else if (gesture_state.scale < 1.0 - config.gesture_pinch_threshold) {
        dir = GESTURE_DIR_IN;
    }
    
    if (dir != GESTURE_DIR_NONE) {
        struct gesture_action *action = find_touchpad_action(gesture_state.fingers, dir);
        if (action) {
            execute_gesture_action(server, action);
        }
    }
    
    gesture_state.active = false;
}

void gesture_hold_begin(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_hold_begin);
    struct wlr_pointer_hold_begin_event *event = data;
    
    gesture_state.active = true;
    gesture_state.type = GESTURE_HOLD;
    gesture_state.fingers = event->fingers;
}

void gesture_hold_end(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, gesture_hold_end);
    (void)server;
    (void)data;
    
    gesture_state.active = false;
}

/* Mouse gestures */
void mouse_gesture_begin(struct server *server, uint32_t button, uint32_t mods) {
    gesture_state.mouse_active = true;
    gesture_state.mouse_button = button;
    gesture_state.mouse_mods = mods;
    gesture_state.mouse_start_x = server->cursor->x;
    gesture_state.mouse_start_y = server->cursor->y;
    gesture_state.mouse_dx = 0;
    gesture_state.mouse_dy = 0;
    
    fprintf(stderr, "[GESTURE] Mouse gesture begin - button: %u, mods: 0x%x, pos: %.1f,%.1f\n",
            button, mods, server->cursor->x, server->cursor->y);
}

void mouse_gesture_update(struct server *server, double dx, double dy) {
    (void)server;
    if (!gesture_state.mouse_active) return;
    
    gesture_state.mouse_dx += dx;
    gesture_state.mouse_dy += dy;
    
    fprintf(stderr, "[GESTURE] Mouse update - delta: %.1f,%.1f, total: %.1f,%.1f\n",
            dx, dy, gesture_state.mouse_dx, gesture_state.mouse_dy);
}

bool mouse_gesture_end(struct server *server) {
    if (!gesture_state.mouse_active) return false;
    
    bool handled = false;
    
    // Determine direction
    enum gesture_direction dir = GESTURE_DIR_NONE;
    double total_dx = gesture_state.mouse_dx;
    double total_dy = gesture_state.mouse_dy;
    
    fprintf(stderr, "[GESTURE] Mouse end - total delta: %.1f,%.1f, threshold: %.1f\n",
            total_dx, total_dy, config.gesture_mouse_threshold);
    
    if (fabs(total_dx) > fabs(total_dy)) {
        if (total_dx > config.gesture_mouse_threshold) {
            dir = GESTURE_DIR_RIGHT;
        } else if (total_dx < -config.gesture_mouse_threshold) {
            dir = GESTURE_DIR_LEFT;
        }
    } else {
        if (total_dy > config.gesture_mouse_threshold) {
            dir = GESTURE_DIR_DOWN;
        } else if (total_dy < -config.gesture_mouse_threshold) {
            dir = GESTURE_DIR_UP;
        }
    }
    
    const char *dir_str[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT", "IN", "OUT"};
    fprintf(stderr, "[GESTURE] Direction detected: %s\n", dir_str[dir]);
    
    if (dir != GESTURE_DIR_NONE) {
        // Find matching mouse gesture
        fprintf(stderr, "[GESTURE] Searching %d configured gestures...\n", config.gesture_mouse_count);
        for (int i = 0; i < config.gesture_mouse_count; i++) {
            struct gesture_mouse *g = &config.gesture_mouse[i];
            fprintf(stderr, "[GESTURE] Config %d - button: %u, dir: %s, mods: 0x%x\n",
                    i, g->button, dir_str[g->direction], g->modifiers);
            fprintf(stderr, "[GESTURE] Checking - button match: %d, dir match: %d, mods match: %d\n",
                    g->button == gesture_state.mouse_button,
                    g->direction == dir,
                    check_modifiers(gesture_state.mouse_mods, g->modifiers));
            
            if (g->button == gesture_state.mouse_button &&
                g->direction == dir &&
                check_modifiers(gesture_state.mouse_mods, g->modifiers)) {
                fprintf(stderr, "[GESTURE] MATCH! Executing action type: %d\n", g->action.type);
                execute_gesture_action(server, &g->action);
                handled = true;
                break;
            }
        }
        if (!handled) {
            fprintf(stderr, "[GESTURE] No matching gesture found\n");
        }
    }
    
    gesture_state.mouse_active = false;
    return handled;
}

bool mouse_gesture_check(uint32_t button, uint32_t mods) {
    fprintf(stderr, "[GESTURE] Check - button: %u, mods: 0x%x, count: %d\n",
            button, mods, config.gesture_mouse_count);
    
    for (int i = 0; i < config.gesture_mouse_count; i++) {
        fprintf(stderr, "[GESTURE] Check %d - button: %u, mods: 0x%x, match: %d\n",
                i, config.gesture_mouse[i].button, config.gesture_mouse[i].modifiers,
                check_modifiers(mods, config.gesture_mouse[i].modifiers));
        
        if (config.gesture_mouse[i].button == button &&
            check_modifiers(mods, config.gesture_mouse[i].modifiers)) {
            fprintf(stderr, "[GESTURE] Check MATCHED!\n");
            return true;
        }
    }
    fprintf(stderr, "[GESTURE] Check - no match found\n");
    return false;
}
