#define _POSIX_C_SOURCE 200809L

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/input-event-codes.h>

struct cursor_state cursor_state = {0};

/*
 * ============================================================================
 * WINDOW ARRANGEMENT
 * ============================================================================
 */

static void tile_output(struct server *server, struct output *output) {
    int count = 0;
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (!toplevel->floating && !toplevel->fullscreen &&
            toplevel->workspace == server->current_workspace) {
            count++;
        }
    }
    
    if (count == 0) return;
    
    int width = output->wlr_output->width;
    int height = output->wlr_output->height;
    int gaps = config.gaps_inner;
    int outer = config.gaps_outer;
    int master_count = config.master_count;
    float master_ratio = config.master_ratio;
    
    int master_width;
    if (count <= master_count) {
        master_width = width - outer * 2;
    } else {
        master_width = (int)((width - outer * 2 - gaps) * master_ratio);
    }
    
    int master_i = 0;
    int stack_i = 0;
    int stack_count = count > master_count ? count - master_count : 0;
    
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (toplevel->floating || toplevel->fullscreen ||
            toplevel->workspace != server->current_workspace) {
            continue;
        }
        
        int x, y, w, h;
        
        if (master_i < master_count) {
            // Master area
            int this_master_count = count < master_count ? count : master_count;
            x = outer;
            w = master_width;
            h = (height - outer * 2 - gaps * (this_master_count - 1)) / this_master_count;
            y = outer + master_i * (h + gaps);
            
            // Account for decorations
            if (config.decor.enabled && toplevel->decor.tree) {
                h = h > config.decor.height ? h - config.decor.height : h;
            }
            master_i++;
        } else {
            // Stack area
            x = outer + master_width + gaps;
            w = width - outer * 2 - master_width - gaps;
            h = (height - outer * 2 - gaps * (stack_count - 1)) / stack_count;
            y = outer + stack_i * (h + gaps);
            
            // Account for decorations
            if (config.decor.enabled && toplevel->decor.tree) {
                h = h > config.decor.height ? h - config.decor.height : h;
            }
            stack_i++;
        }
        
        // Ensure minimum size
        if (w < 100) w = 100;
        if (h < 50) h = 50;
        
        if (config.anim.enabled && config.anim.window_move != ANIM_NONE) {
            anim_start(toplevel, config.anim.window_move, x, y, w, h, NULL, NULL);
            anim_schedule_update(server);
        } else {
            struct wlr_scene_node *node = toplevel->decor.tree ?
                &toplevel->decor.tree->node : &toplevel->scene_tree->node;
            wlr_scene_node_set_position(node, x, y);
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, w, h);
        }
    }
}

void arrange_windows(struct server *server) {
    if (server->mode == MODE_FLOATING) {
        return;
    }
    
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        tile_output(server, output);
    }
}

static void toggle_mode(struct server *server) {
    if (server->mode == MODE_TILING) {
        server->mode = MODE_FLOATING;
        struct toplevel *t;
        wl_list_for_each(t, &server->toplevels, link) {
            struct wlr_scene_node *node = t->decor.tree ?
                &t->decor.tree->node : &t->scene_tree->node;
            t->saved_x = node->x;
            t->saved_y = node->y;
            struct wlr_box geo;
            wlr_xdg_surface_get_geometry(t->xdg_toplevel->base, &geo);
            t->saved_width = geo.width;
            t->saved_height = geo.height;
        }
        printf("Switched to FLOATING mode\n");
    } else {
        server->mode = MODE_TILING;
        arrange_windows(server);
        printf("Switched to TILING mode\n");
    }
}

/*
 * ============================================================================
 * FOCUS MANAGEMENT
 * ============================================================================
 */

struct toplevel *get_focused_toplevel(struct server *server) {
    struct wlr_surface *focused_surface = server->seat->keyboard_state.focused_surface;
    if (!focused_surface) return NULL;
    
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (toplevel->xdg_toplevel->base->surface == focused_surface) {
            return toplevel;
        }
    }
    return NULL;
}

void focus_toplevel(struct toplevel *toplevel) {
    if (!toplevel) return;
    
    struct server *server = toplevel->server;
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(server->seat);
    
    if (toplevel->decor.tree) {
        wlr_scene_node_raise_to_top(&toplevel->decor.tree->node);
    } else {
        wlr_scene_node_raise_to_top(&toplevel->scene_tree->node);
    }
    
    struct toplevel *t;
    wl_list_for_each(t, &server->toplevels, link) {
        decor_update(t, t == toplevel);
    }
    
    if (kb) {
        wlr_seat_keyboard_notify_enter(server->seat,
            toplevel->xdg_toplevel->base->surface,
            kb->keycodes, kb->num_keycodes, &kb->modifiers);
    }
}

static struct toplevel *get_toplevel_in_direction(struct server *server, 
                                                   struct toplevel *from,
                                                   int dx, int dy) {
    if (!from) return NULL;
    
    struct wlr_scene_node *from_node = from->decor.tree ?
        &from->decor.tree->node : &from->scene_tree->node;
    int from_x = from_node->x;
    int from_y = from_node->y;
    
    struct toplevel *best = NULL;
    int best_dist = INT32_MAX;
    
    struct toplevel *t;
    wl_list_for_each(t, &server->toplevels, link) {
        if (t == from || t->workspace != server->current_workspace) continue;
        
        struct wlr_scene_node *node = t->decor.tree ?
            &t->decor.tree->node : &t->scene_tree->node;
        int tx = node->x;
        int ty = node->y;
        
        bool valid = false;
        if (dx < 0 && tx < from_x) valid = true;
        if (dx > 0 && tx > from_x) valid = true;
        if (dy < 0 && ty < from_y) valid = true;
        if (dy > 0 && ty > from_y) valid = true;
        
        if (!valid) continue;
        
        int dist = abs(tx - from_x) + abs(ty - from_y);
        if (dist < best_dist) {
            best_dist = dist;
            best = t;
        }
    }
    
    return best;
}

/*
 * ============================================================================
 * TOPLEVEL HANDLERS
 * ============================================================================
 */

static void toplevel_map(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, map);
    struct server *server = toplevel->server;
    
    wl_list_insert(&server->toplevels, &toplevel->link);
    toplevel->workspace = server->current_workspace;
    toplevel->opacity = 1.0f;
    
    if (config.decor.enabled) {
        decor_create(toplevel);
        
        struct wlr_box geo;
        wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
        decor_set_size(toplevel, geo.width > 0 ? geo.width : 800);
        decor_update(toplevel, true);
    }
    
    apply_window_rules(toplevel);
    
    if (server->mode == MODE_FLOATING || toplevel->floating) {
        struct output *output;
        wl_list_for_each(output, &server->outputs, link) {
            struct wlr_box geo;
            wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
            int x = (output->wlr_output->width - geo.width) / 2;
            int y = (output->wlr_output->height - geo.height) / 2;
            
            if (config.anim.enabled && config.anim.window_open != ANIM_NONE) {
                anim_start(toplevel, config.anim.window_open, x, y, 
                          geo.width, geo.height, NULL, NULL);
                anim_schedule_update(server);
            } else {
                struct wlr_scene_node *node = toplevel->decor.tree ?
                    &toplevel->decor.tree->node : &toplevel->scene_tree->node;
                wlr_scene_node_set_position(node, x, y);
            }
            break;
        }
    } else {
        arrange_windows(server);
    }
    
    focus_toplevel(toplevel);
}

static void toplevel_unmap(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, unmap);
    struct server *server = toplevel->server;
    
    wl_list_remove(&toplevel->link);
    
    if (cursor_state.toplevel == toplevel) {
        cursor_state.mode = CURSOR_NORMAL;
        cursor_state.toplevel = NULL;
    }
    
    arrange_windows(server);
    
    if (!wl_list_empty(&server->toplevels)) {
        struct toplevel *next = wl_container_of(server->toplevels.next, next, link);
        focus_toplevel(next);
    }
}

static void toplevel_commit(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, commit);
    
    if (config.decor.enabled && toplevel->decor.tree) {
        struct wlr_box geo;
        wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
        if (geo.width > 0 && geo.width != toplevel->decor.width) {
            decor_set_size(toplevel, geo.width);
        }
    }
}

static void toplevel_destroy(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, destroy);
    
    decor_destroy(toplevel);
    
    wl_list_remove(&toplevel->map.link);
    wl_list_remove(&toplevel->unmap.link);
    wl_list_remove(&toplevel->commit.link);
    wl_list_remove(&toplevel->destroy.link);
    wl_list_remove(&toplevel->request_fullscreen.link);
    wl_list_remove(&toplevel->request_minimize.link);
    free(toplevel);
}

static void toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, request_fullscreen);
    
    if (toplevel->xdg_toplevel->requested.fullscreen) {
        toplevel->fullscreen = true;
        
        struct wlr_scene_node *node = toplevel->decor.tree ?
            &toplevel->decor.tree->node : &toplevel->scene_tree->node;
        toplevel->saved_x = node->x;
        toplevel->saved_y = node->y;
        struct wlr_box geo;
        wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
        toplevel->saved_width = geo.width;
        toplevel->saved_height = geo.height;
        
        if (toplevel->decor.tree) {
            wlr_scene_node_set_enabled(&toplevel->decor.tree->node, false);
            wlr_scene_node_set_enabled(&toplevel->scene_tree->node, true);
        }
        
        struct output *output;
        wl_list_for_each(output, &toplevel->server->outputs, link) {
            wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, 0);
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel,
                output->wlr_output->width, output->wlr_output->height);
            break;
        }
        wlr_xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, true);
    } else {
        toplevel->fullscreen = false;
        wlr_xdg_toplevel_set_fullscreen(toplevel->xdg_toplevel, false);
        
        if (toplevel->decor.tree) {
            wlr_scene_node_set_enabled(&toplevel->decor.tree->node, true);
        }
        
        if (toplevel->server->mode == MODE_TILING && !toplevel->floating) {
            arrange_windows(toplevel->server);
        } else {
            struct wlr_scene_node *node = toplevel->decor.tree ?
                &toplevel->decor.tree->node : &toplevel->scene_tree->node;
            wlr_scene_node_set_position(node, toplevel->saved_x, toplevel->saved_y);
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel,
                toplevel->saved_width, toplevel->saved_height);
        }
    }
}

static void toplevel_request_minimize(struct wl_listener *listener, void *data) {
    struct toplevel *toplevel = wl_container_of(listener, toplevel, request_minimize);
    
    toplevel->minimized = !toplevel->minimized;
    
    struct wlr_scene_node *node = toplevel->decor.tree ?
        &toplevel->decor.tree->node : &toplevel->scene_tree->node;
    wlr_scene_node_set_enabled(node, !toplevel->minimized);
    
    if (toplevel->minimized) {
        if (!wl_list_empty(&toplevel->server->toplevels)) {
            struct toplevel *next;
            wl_list_for_each(next, &toplevel->server->toplevels, link) {
                if (next != toplevel && !next->minimized) {
                    focus_toplevel(next);
                    break;
                }
            }
        }
    }
}

void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_xdg_toplevel);
    struct wlr_xdg_toplevel *xdg_toplevel = data;

    struct toplevel *toplevel = calloc(1, sizeof(*toplevel));
    toplevel->server = server;
    toplevel->xdg_toplevel = xdg_toplevel;
    toplevel->scene_tree = wlr_scene_xdg_surface_create(
        server->layer_windows, xdg_toplevel->base);
    toplevel->scene_tree->node.data = toplevel;
    xdg_toplevel->base->data = toplevel->scene_tree;
    
    toplevel->floating = false;
    toplevel->fullscreen = false;
    toplevel->minimized = false;
    toplevel->workspace = server->current_workspace;
    toplevel->opacity = 1.0f;

    toplevel->map.notify = toplevel_map;
    wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);

    toplevel->unmap.notify = toplevel_unmap;
    wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);

    toplevel->commit.notify = toplevel_commit;
    wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);

    toplevel->destroy.notify = toplevel_destroy;
    wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);
    
    toplevel->request_fullscreen.notify = toplevel_request_fullscreen;
    wl_signal_add(&xdg_toplevel->events.request_fullscreen, &toplevel->request_fullscreen);
    
    toplevel->request_minimize.notify = toplevel_request_minimize;
    wl_signal_add(&xdg_toplevel->events.request_minimize, &toplevel->request_minimize);

    wlr_xdg_toplevel_set_size(xdg_toplevel, 0, 0);
}

/*
 * ============================================================================
 * CURSOR BUTTON HANDLER
 * ============================================================================
 */

void cursor_button(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_pointer_button_event *event = data;

    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
        if (cursor_state.mode == CURSOR_RESIZE) {
            wlr_xdg_toplevel_set_resizing(cursor_state.toplevel->xdg_toplevel, false);
        }
        cursor_state.mode = CURSOR_NORMAL;
        cursor_state.toplevel = NULL;
        wlr_seat_pointer_notify_button(server->seat, event->time_msec, 
            event->button, event->state);
        return;
    }

    double sx, sy;
    struct wlr_surface *surface = NULL;
    struct toplevel *toplevel = toplevel_at(server, server->cursor->x,
        server->cursor->y, &surface, &sx, &sy);

    if (!toplevel) {
        wlr_seat_pointer_notify_button(server->seat, event->time_msec,
            event->button, event->state);
        return;
    }

    focus_toplevel(toplevel);

    if (config.decor.enabled && event->button == BTN_LEFT) {
        enum decor_hit hit = decor_hit_test(toplevel, server->cursor->x, server->cursor->y);
        
        if (hit == HIT_CLOSE) {
            wlr_xdg_toplevel_send_close(toplevel->xdg_toplevel);
            return;
        } else if (hit == HIT_MAXIMIZE) {
            handle_keybind(server, ACTION_MAXIMIZE, "");
            return;
        } else if (hit == HIT_MINIMIZE) {
            handle_keybind(server, ACTION_MINIMIZE, "");
            return;
        } else if (hit == HIT_TITLEBAR) {
            if (server->mode == MODE_FLOATING || toplevel->floating) {
                cursor_state.mode = CURSOR_MOVE;
                cursor_state.toplevel = toplevel;
                struct wlr_scene_node *node = toplevel->decor.tree ?
                    &toplevel->decor.tree->node : &toplevel->scene_tree->node;
                cursor_state.grab_x = server->cursor->x - node->x;
                cursor_state.grab_y = server->cursor->y - node->y;
            }
            return;
        } else if (hit >= HIT_RESIZE_TOP && hit <= HIT_RESIZE_BOTTOM_RIGHT) {
            if (server->mode == MODE_FLOATING || toplevel->floating) {
                cursor_state.mode = CURSOR_RESIZE;
                cursor_state.toplevel = toplevel;
                cursor_state.resize_edges = hit;
                
                struct wlr_box geo;
                wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
                struct wlr_scene_node *node = toplevel->decor.tree ?
                    &toplevel->decor.tree->node : &toplevel->scene_tree->node;
                
                cursor_state.grab_box.x = node->x;
                cursor_state.grab_box.y = node->y;
                cursor_state.grab_box.width = geo.width;
                cursor_state.grab_box.height = geo.height;
                
                cursor_state.grab_x = server->cursor->x;
                cursor_state.grab_y = server->cursor->y;
                
                wlr_xdg_toplevel_set_resizing(toplevel->xdg_toplevel, true);
            }
            return;
        }
    }

    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
    bool alt_pressed = keyboard && 
        (wlr_keyboard_get_modifiers(keyboard) & WLR_MODIFIER_ALT);

    if (alt_pressed && event->button == BTN_LEFT) {
        if (server->mode == MODE_FLOATING || toplevel->floating) {
            cursor_state.mode = CURSOR_MOVE;
            cursor_state.toplevel = toplevel;
            struct wlr_scene_node *node = toplevel->decor.tree ?
                &toplevel->decor.tree->node : &toplevel->scene_tree->node;
            cursor_state.grab_x = server->cursor->x - node->x;
            cursor_state.grab_y = server->cursor->y - node->y;
        }
        return;
    }

    if (alt_pressed && event->button == BTN_RIGHT) {
        if (server->mode == MODE_FLOATING || toplevel->floating) {
            cursor_state.mode = CURSOR_RESIZE;
            cursor_state.toplevel = toplevel;
            cursor_state.resize_edges = HIT_RESIZE_BOTTOM_RIGHT;
            
            struct wlr_box geo;
            wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
            struct wlr_scene_node *node = toplevel->decor.tree ?
                &toplevel->decor.tree->node : &toplevel->scene_tree->node;
            
            cursor_state.grab_box.x = node->x;
            cursor_state.grab_box.y = node->y;
            cursor_state.grab_box.width = geo.width;
            cursor_state.grab_box.height = geo.height;
            
            cursor_state.grab_x = server->cursor->x;
            cursor_state.grab_y = server->cursor->y;
            
            wlr_xdg_toplevel_set_resizing(toplevel->xdg_toplevel, true);
        }
        return;
    }

    wlr_seat_pointer_notify_button(server->seat, event->time_msec,
        event->button, event->state);
}

/*
 * ============================================================================
 * KEYBIND HANDLER
 * ============================================================================
 */

bool handle_keybind(struct server *server, enum keybind_action action, const char *arg) {
    struct toplevel *focused = get_focused_toplevel(server);
    
    switch (action) {
    
    case ACTION_SPAWN:
        if (fork() == 0) {
            execl("/bin/sh", "sh", "-c", arg, NULL);
            _exit(0);
        }
        return true;
    
    case ACTION_CLOSE:
        if (focused) {
            wlr_xdg_toplevel_send_close(focused->xdg_toplevel);
        }
        return true;
        
    case ACTION_FULLSCREEN:
        if (focused) {
            focused->fullscreen = !focused->fullscreen;
            
            if (focused->fullscreen) {
                struct wlr_scene_node *node = focused->decor.tree ?
                    &focused->decor.tree->node : &focused->scene_tree->node;
                focused->saved_x = node->x;
                focused->saved_y = node->y;
                struct wlr_box geo;
                wlr_xdg_surface_get_geometry(focused->xdg_toplevel->base, &geo);
                focused->saved_width = geo.width;
                focused->saved_height = geo.height;
                
                if (focused->decor.tree) {
                    wlr_scene_node_set_enabled(&focused->decor.tree->node, false);
                    wlr_scene_node_set_enabled(&focused->scene_tree->node, true);
                }
                
                struct output *output;
                wl_list_for_each(output, &server->outputs, link) {
                    wlr_scene_node_set_position(&focused->scene_tree->node, 0, 0);
                    wlr_xdg_toplevel_set_size(focused->xdg_toplevel,
                        output->wlr_output->width, output->wlr_output->height);
                    break;
                }
            } else {
                if (focused->decor.tree) {
                    wlr_scene_node_set_enabled(&focused->decor.tree->node, true);
                }
                
                if (server->mode == MODE_TILING && !focused->floating) {
                    arrange_windows(server);
                } else {
                    struct wlr_scene_node *node = focused->decor.tree ?
                        &focused->decor.tree->node : &focused->scene_tree->node;
                    wlr_scene_node_set_position(node, focused->saved_x, focused->saved_y);
                    wlr_xdg_toplevel_set_size(focused->xdg_toplevel,
                        focused->saved_width, focused->saved_height);
                }
            }
            wlr_xdg_toplevel_set_fullscreen(focused->xdg_toplevel, focused->fullscreen);
        }
        return true;
        
    case ACTION_TOGGLE_FLOATING:
        if (focused) {
            focused->floating = !focused->floating;
            
            if (focused->floating) {
                focused->saved_x = focused->scene_tree->node.x;
                focused->saved_y = focused->scene_tree->node.y;
                struct wlr_box geo;
                wlr_xdg_surface_get_geometry(focused->xdg_toplevel->base, &geo);
                focused->saved_width = geo.width;
                focused->saved_height = geo.height;
            }
            arrange_windows(server);
        }
        return true;
    
    case ACTION_MINIMIZE:
        if (focused) {
            focused->minimized = !focused->minimized;
            struct wlr_scene_node *node = focused->decor.tree ?
                &focused->decor.tree->node : &focused->scene_tree->node;
            
            if (config.anim.enabled && config.anim.window_close != ANIM_NONE) {
                if (focused->minimized) {
                    anim_start_opacity(focused, 0.0f, NULL, NULL);
                    anim_schedule_update(server);
                } else {
                    wlr_scene_node_set_enabled(node, true);
                    anim_start_opacity(focused, 1.0f, NULL, NULL);
                    anim_schedule_update(server);
                }
            } else {
                wlr_scene_node_set_enabled(node, !focused->minimized);
            }
            
            if (focused->minimized && !wl_list_empty(&server->toplevels)) {
                struct toplevel *next;
                wl_list_for_each(next, &server->toplevels, link) {
                    if (next != focused && !next->minimized) {
                        focus_toplevel(next);
                        break;
                    }
                }
            }
        }
        return true;
    
    case ACTION_MAXIMIZE:
        if (focused) {
            focused->maximized = !focused->maximized;
            
            if (focused->maximized) {
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
                    
                    if (config.decor.enabled && focused->decor.tree) {
                        h -= config.decor.height;
                    }
                    
                    if (config.anim.enabled && config.anim.window_resize != ANIM_NONE) {
                        anim_start(focused, config.anim.window_resize,
                                  config.gaps_outer, config.gaps_outer, w, h, NULL, NULL);
                        anim_schedule_update(server);
                    } else {
                        wlr_scene_node_set_position(node, config.gaps_outer, config.gaps_outer);
                        wlr_xdg_toplevel_set_size(focused->xdg_toplevel, w, h);
                    }
                    break;
                }
            } else {
                struct wlr_scene_node *node = focused->decor.tree ?
                    &focused->decor.tree->node : &focused->scene_tree->node;
                wlr_scene_node_set_position(node, focused->pre_max_x, focused->pre_max_y);
                wlr_xdg_toplevel_set_size(focused->xdg_toplevel, 
                    focused->pre_max_width, focused->pre_max_height);
            }
        }
        return true;
    
    case ACTION_SNAP_LEFT:
        if (focused && (server->mode == MODE_FLOATING || focused->floating)) {
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
        }
        return true;
    
    case ACTION_SNAP_RIGHT:
        if (focused && (server->mode == MODE_FLOATING || focused->floating)) {
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
        }
        return true;
    
    case ACTION_CENTER_WINDOW:
        if (focused && (server->mode == MODE_FLOATING || focused->floating)) {
            struct output *output;
            wl_list_for_each(output, &server->outputs, link) {
                struct wlr_box geo;
                wlr_xdg_surface_get_geometry(focused->xdg_toplevel->base, &geo);
                int x = (output->wlr_output->width - geo.width) / 2;
                int y = (output->wlr_output->height - geo.height) / 2;
                
                struct wlr_scene_node *node = focused->decor.tree ?
                    &focused->decor.tree->node : &focused->scene_tree->node;
                wlr_scene_node_set_position(node, x, y);
                break;
            }
        }
        return true;
    
    case ACTION_FOCUS_LEFT: {
        struct toplevel *t = get_toplevel_in_direction(server, focused, -1, 0);
        if (t) focus_toplevel(t);
        return true;
    }
    case ACTION_FOCUS_RIGHT: {
        struct toplevel *t = get_toplevel_in_direction(server, focused, 1, 0);
        if (t) focus_toplevel(t);
        return true;
    }
    case ACTION_FOCUS_UP: {
        struct toplevel *t = get_toplevel_in_direction(server, focused, 0, -1);
        if (t) focus_toplevel(t);
        return true;
    }
    case ACTION_FOCUS_DOWN: {
        struct toplevel *t = get_toplevel_in_direction(server, focused, 0, 1);
        if (t) focus_toplevel(t);
        return true;
    }
        
    case ACTION_FOCUS_NEXT: {
        if (wl_list_empty(&server->toplevels)) return true;
        if (!focused) {
            struct toplevel *first = wl_container_of(server->toplevels.next, first, link);
            focus_toplevel(first);
        } else {
            struct wl_list *next = focused->link.next;
            if (next == &server->toplevels) next = server->toplevels.next;
            struct toplevel *next_toplevel = wl_container_of(next, next_toplevel, link);
            focus_toplevel(next_toplevel);
        }
        return true;
    }
    
    case ACTION_FOCUS_PREV: {
        if (wl_list_empty(&server->toplevels)) return true;
        if (!focused) {
            struct toplevel *last = wl_container_of(server->toplevels.prev, last, link);
            focus_toplevel(last);
        } else {
            struct wl_list *prev = focused->link.prev;
            if (prev == &server->toplevels) prev = server->toplevels.prev;
            struct toplevel *prev_toplevel = wl_container_of(prev, prev_toplevel, link);
            focus_toplevel(prev_toplevel);
        }
        return true;
    }
    
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_UP:
    case ACTION_MOVE_DOWN:
        if (focused && (server->mode == MODE_FLOATING || focused->floating)) {
            // Floating mode - move by step
            int dx = 0, dy = 0;
            if (action == ACTION_MOVE_LEFT) dx = -config.move_step;
            if (action == ACTION_MOVE_RIGHT) dx = config.move_step;
            if (action == ACTION_MOVE_UP) dy = -config.move_step;
            if (action == ACTION_MOVE_DOWN) dy = config.move_step;
            
            struct wlr_scene_node *node = focused->decor.tree ?
                &focused->decor.tree->node : &focused->scene_tree->node;
            wlr_scene_node_set_position(node, node->x + dx, node->y + dy);
        } else if (focused && server->mode == MODE_TILING && !focused->floating) {
            // Tiling mode - swap windows in direction
            int dx = 0, dy = 0;
            if (action == ACTION_MOVE_LEFT) dx = -1;
            if (action == ACTION_MOVE_RIGHT) dx = 1;
            if (action == ACTION_MOVE_UP) dy = -1;
            if (action == ACTION_MOVE_DOWN) dy = 1;
            
            struct toplevel *target = get_toplevel_in_direction(server, focused, dx, dy);
            if (target && target != focused) {
                // Swap positions in the list to change tiling order
                wl_list_remove(&focused->link);
                if (dy < 0 || dx < 0) {
                    // Move before target
                    wl_list_insert(&target->link, &focused->link);
                } else {
                    // Move after target
                    wl_list_insert(target->link.next, &focused->link);
                }
                arrange_windows(server);
            }
        }
        return true;
    
    case ACTION_RESIZE_GROW_WIDTH:
    case ACTION_RESIZE_SHRINK_WIDTH:
    case ACTION_RESIZE_GROW_HEIGHT:
    case ACTION_RESIZE_SHRINK_HEIGHT:
        if (focused && (server->mode == MODE_FLOATING || focused->floating)) {
            struct wlr_box geo;
            wlr_xdg_surface_get_geometry(focused->xdg_toplevel->base, &geo);
            int w = geo.width, h = geo.height;
            if (action == ACTION_RESIZE_GROW_WIDTH) w += config.resize_step;
            if (action == ACTION_RESIZE_SHRINK_WIDTH) w -= config.resize_step;
            if (action == ACTION_RESIZE_GROW_HEIGHT) h += config.resize_step;
            if (action == ACTION_RESIZE_SHRINK_HEIGHT) h -= config.resize_step;
            if (w < 100) w = 100;
            if (h < 100) h = 100;
            wlr_xdg_toplevel_set_size(focused->xdg_toplevel, w, h);
        }
        return true;
    
    case ACTION_WORKSPACE: {
        int ws = atoi(arg);
        if (ws >= 1 && ws <= MAX_WORKSPACES) {
            server->current_workspace = ws;
            struct toplevel *t;
            wl_list_for_each(t, &server->toplevels, link) {
                bool visible = (t->workspace == ws);
                struct wlr_scene_node *node = t->decor.tree ?
                    &t->decor.tree->node : &t->scene_tree->node;
                wlr_scene_node_set_enabled(node, visible);
            }
            arrange_windows(server);
            printf("Switched to workspace %d\n", ws);
        }
        return true;
    }
    
    case ACTION_MOVE_TO_WORKSPACE: {
        int ws = atoi(arg);
        if (focused && ws >= 1 && ws <= MAX_WORKSPACES) {
            focused->workspace = ws;
            if (ws != server->current_workspace) {
                struct wlr_scene_node *node = focused->decor.tree ?
                    &focused->decor.tree->node : &focused->scene_tree->node;
                wlr_scene_node_set_enabled(node, false);
            }
            arrange_windows(server);
            printf("Moved window to workspace %d\n", ws);
        }
        return true;
    }
    
    case ACTION_WORKSPACE_NEXT: {
        int ws = server->current_workspace + 1;
        if (ws > MAX_WORKSPACES) ws = 1;
        server->current_workspace = ws;
        struct toplevel *t;
        wl_list_for_each(t, &server->toplevels, link) {
            struct wlr_scene_node *node = t->decor.tree ?
                &t->decor.tree->node : &t->scene_tree->node;
            wlr_scene_node_set_enabled(node, t->workspace == ws);
        }
        arrange_windows(server);
        return true;
    }
        
    case ACTION_WORKSPACE_PREV: {
        int ws = server->current_workspace - 1;
        if (ws < 1) ws = MAX_WORKSPACES;
        server->current_workspace = ws;
        struct toplevel *t;
        wl_list_for_each(t, &server->toplevels, link) {
            struct wlr_scene_node *node = t->decor.tree ?
                &t->decor.tree->node : &t->scene_tree->node;
            wlr_scene_node_set_enabled(node, t->workspace == ws);
        }
        arrange_windows(server);
        return true;
    }
    
    case ACTION_MODE_TILING:
        server->mode = MODE_TILING;
        arrange_windows(server);
        printf("Switched to TILING mode\n");
        return true;
        
    case ACTION_MODE_FLOATING:
        server->mode = MODE_FLOATING;
        printf("Switched to FLOATING mode\n");
        return true;
        
    case ACTION_TOGGLE_MODE:
        toggle_mode(server);
        return true;
    
    case ACTION_INC_MASTER_COUNT:
        config.master_count++;
        arrange_windows(server);
        return true;
        
    case ACTION_DEC_MASTER_COUNT:
        if (config.master_count > 1) {
            config.master_count--;
            arrange_windows(server);
        }
        return true;
        
    case ACTION_INC_MASTER_RATIO:
        config.master_ratio += 0.05f;
        if (config.master_ratio > 0.9f) config.master_ratio = 0.9f;
        arrange_windows(server);
        return true;
        
    case ACTION_DEC_MASTER_RATIO:
        config.master_ratio -= 0.05f;
        if (config.master_ratio < 0.1f) config.master_ratio = 0.1f;
        arrange_windows(server);
        return true;
    
    case ACTION_RELOAD_CONFIG:
        config_reload();
        arrange_windows(server);
        return true;
        
    case ACTION_EXIT:
        wl_display_terminate(server->display);
        return true;
        
    default:
        return false;
    }
}
