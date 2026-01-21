#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>

/* Output handlers */
static void output_frame(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, frame);
  wlr_scene_output_commit(output->scene_output, NULL);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(output->scene_output, &now);
}

static void layer_surface_map(struct wl_listener *listener, void *data) {
    struct layer_surface *layer = wl_container_of(listener, layer, map);
    
    printf("Layer MAPPED: %s anchor=%d exclusive=%d size=%dx%d\n",
           layer->wlr_layer->namespace,
           layer->wlr_layer->current.anchor,
           layer->wlr_layer->current.exclusive_zone,
           layer->wlr_layer->surface->current.width,
           layer->wlr_layer->surface->current.height);
    
    wlr_surface_send_enter(layer->wlr_layer->surface, layer->wlr_layer->output);
}

static void layer_surface_unmap(struct wl_listener *listener, void *data) {
    struct layer_surface *layer = wl_container_of(listener, layer, unmap);
    printf("Layer unmapped: %s\n", layer->wlr_layer->namespace);
}

static void layer_surface_commit(struct wl_listener *listener, void *data) {
    struct layer_surface *layer = wl_container_of(listener, layer, commit);
    struct wlr_output *output = layer->wlr_layer->output;
    
    if (!output) return;
    
    struct wlr_box full_box = { 
        .x = 0, .y = 0, 
        .width = output->width, 
        .height = output->height 
    };
    
    wlr_scene_layer_surface_v1_configure(layer->scene_layer, &full_box, &full_box);
}

static void layer_surface_destroy(struct wl_listener *listener, void *data) {
    struct layer_surface *layer = wl_container_of(listener, layer, destroy);
    printf("Layer destroyed: %s\n", layer->wlr_layer->namespace);
    
    wl_list_remove(&layer->map.link);
    wl_list_remove(&layer->unmap.link);
    wl_list_remove(&layer->commit.link);
    wl_list_remove(&layer->destroy.link);
    wl_list_remove(&layer->link);
    free(layer);
}

void server_new_layer_surface(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_layer_surface);
    struct wlr_layer_surface_v1 *wlr_layer = data;

    printf("\n=== NEW LAYER SURFACE ===\n");
    printf("Namespace: %s\n", wlr_layer->namespace);
    printf("Layer: %d\n", wlr_layer->pending.layer);

    if (!wlr_layer->output) {
        struct output *output;
        wl_list_for_each(output, &server->outputs, link) {
            wlr_layer->output = output->wlr_output;
            break;
        }
    }
    
    if (!wlr_layer->output) {
        wlr_layer_surface_v1_destroy(wlr_layer);
        return;
    }

    struct layer_surface *layer = calloc(1, sizeof(*layer));
    if (!layer) {
        wlr_layer_surface_v1_destroy(wlr_layer);
        return;
    }
    
    layer->server = server;
    layer->wlr_layer = wlr_layer;
    wlr_layer->data = layer;

    struct wlr_scene_tree *parent;
    switch (wlr_layer->pending.layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
        parent = server->layer_bg;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
        parent = server->layer_bottom;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
        parent = server->layer_top;
        break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
        parent = server->layer_overlay;
        break;
    default:
        parent = server->layer_top;
    }

    layer->scene_layer = wlr_scene_layer_surface_v1_create(parent, wlr_layer);
    
    if (!layer->scene_layer) {
        printf("ERROR: Failed to create scene layer surface!\n");
        free(layer);
        wlr_layer_surface_v1_destroy(wlr_layer);
        return;
    }
    
    layer->scene_tree = layer->scene_layer->tree;
    layer->scene_tree->node.data = layer;

    layer->map.notify = layer_surface_map;
    wl_signal_add(&wlr_layer->surface->events.map, &layer->map);

    layer->unmap.notify = layer_surface_unmap;
    wl_signal_add(&wlr_layer->surface->events.unmap, &layer->unmap);

    layer->commit.notify = layer_surface_commit;
    wl_signal_add(&wlr_layer->surface->events.commit, &layer->commit);

    layer->destroy.notify = layer_surface_destroy;
    wl_signal_add(&wlr_layer->events.destroy, &layer->destroy);

    wl_list_insert(&server->layers, &layer->link);

    printf("=========================\n\n");
}

static void output_request_state(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, request_state);
  struct wlr_output_event_request_state *event = data;
  wlr_output_commit_state(output->wlr_output, event->state);
}

static void output_destroy(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, destroy);
  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);
  free(output);
}

void server_new_output(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, new_output);
  struct wlr_output *wlr_output = data;

  wlr_output_init_render(wlr_output, server->allocator, server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
  if (mode) {
    wlr_output_state_set_mode(&state, mode);
  }
  wlr_output_commit_state(wlr_output, &state);
  wlr_output_state_finish(&state);

  struct output *output = calloc(1, sizeof(*output));
  output->server = server;
  output->wlr_output = wlr_output;

  output->frame.notify = output_frame;
  wl_signal_add(&wlr_output->events.frame, &output->frame);

  output->request_state.notify = output_request_state;
  wl_signal_add(&wlr_output->events.request_state, &output->request_state);

  output->destroy.notify = output_destroy;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);

  struct wlr_output_layout_output *l_output =
      wlr_output_layout_add_auto(server->output_layout, wlr_output);
  output->scene_output = wlr_scene_output_create(server->scene, wlr_output);
  wlr_scene_output_layout_add_output(server->scene_layout, l_output,
                                     output->scene_output);

  wl_list_insert(&server->outputs, &output->link);
}

/* Find layer surface at coordinates */
static struct layer_surface *layer_surface_at(struct server *server, 
                                               double lx, double ly,
                                               struct wlr_surface **surface,
                                               double *sx, double *sy) {
    struct wlr_scene_node *node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
    if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
        return NULL;
    }
    
    struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
    struct wlr_scene_surface *scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
    if (!scene_surface) {
        return NULL;
    }
    
    *surface = scene_surface->surface;
    
    struct wlr_scene_tree *tree = node->parent;
    while (tree) {
        if (tree->node.data) {
            struct layer_surface *layer;
            wl_list_for_each(layer, &server->layers, link) {
                if (layer->scene_tree == tree) {
                    return layer;
                }
            }
        }
        tree = tree->node.parent;
    }
    
    return NULL;
}

static void process_cursor_motion(struct server *server, uint32_t time) {
    double sx, sy;
    struct wlr_surface *surface = NULL;
    
    /* Check layer surfaces first (panels, etc) */
    struct layer_surface *layer = layer_surface_at(
        server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    
    if (layer && surface) {
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
        return;
    }
    
    /* Then check toplevels */
    struct toplevel *toplevel = toplevel_at(
        server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

    if (!toplevel) {
        wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
        wlr_seat_pointer_clear_focus(server->seat);
    } else {
        if (config.decor.enabled) {
            decor_update_hover(toplevel, server->cursor->x, server->cursor->y);
            
            enum decor_hit hit = decor_hit_test(toplevel, server->cursor->x, server->cursor->y);
            const char *cursor_name = "default";
            
            switch (hit) {
            case HIT_RESIZE_TOP:
            case HIT_RESIZE_BOTTOM:
                cursor_name = "ns-resize";
                break;
            case HIT_RESIZE_LEFT:
            case HIT_RESIZE_RIGHT:
                cursor_name = "ew-resize";
                break;
            case HIT_RESIZE_TOP_LEFT:
            case HIT_RESIZE_BOTTOM_RIGHT:
                cursor_name = "nwse-resize";
                break;
            case HIT_RESIZE_TOP_RIGHT:
            case HIT_RESIZE_BOTTOM_LEFT:
                cursor_name = "nesw-resize";
                break;
            default:
                break;
            }
            
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, cursor_name);
        }
        
        wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    }
}

void cursor_motion(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_pointer_motion_event *event = data;
    
    if (cursor_state.mode == CURSOR_MOVE) {
        struct wlr_scene_node *node = cursor_state.toplevel->decor.tree ?
            &cursor_state.toplevel->decor.tree->node : 
            &cursor_state.toplevel->scene_tree->node;
        wlr_scene_node_set_position(node,
                                    server->cursor->x - cursor_state.grab_x,
                                    server->cursor->y - cursor_state.grab_y);
        wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
        return;
    }

    if (cursor_state.mode == CURSOR_RESIZE) {
        double dx = server->cursor->x - cursor_state.grab_x;
        double dy = server->cursor->y - cursor_state.grab_y;
        
        int new_x = cursor_state.grab_box.x;
        int new_y = cursor_state.grab_box.y;
        int new_width = cursor_state.grab_box.width;
        int new_height = cursor_state.grab_box.height;
        
        enum decor_hit edge = cursor_state.resize_edges;
        
        if (edge == HIT_RESIZE_LEFT || edge == HIT_RESIZE_TOP_LEFT || 
            edge == HIT_RESIZE_BOTTOM_LEFT) {
            new_x = cursor_state.grab_box.x + dx;
            new_width = cursor_state.grab_box.width - dx;
        } else if (edge == HIT_RESIZE_RIGHT || edge == HIT_RESIZE_TOP_RIGHT || 
                   edge == HIT_RESIZE_BOTTOM_RIGHT) {
            new_width = cursor_state.grab_box.width + dx;
        }
        
        if (edge == HIT_RESIZE_TOP || edge == HIT_RESIZE_TOP_LEFT || 
            edge == HIT_RESIZE_TOP_RIGHT) {
            new_y = cursor_state.grab_box.y + dy;
            new_height = cursor_state.grab_box.height - dy;
        } else if (edge == HIT_RESIZE_BOTTOM || edge == HIT_RESIZE_BOTTOM_LEFT || 
                   edge == HIT_RESIZE_BOTTOM_RIGHT) {
            new_height = cursor_state.grab_box.height + dy;
        }
        
        if (new_width < 100) {
            new_width = 100;
            if (edge == HIT_RESIZE_LEFT || edge == HIT_RESIZE_TOP_LEFT || 
                edge == HIT_RESIZE_BOTTOM_LEFT) {
                new_x = cursor_state.grab_box.x + cursor_state.grab_box.width - 100;
            }
        }
        if (new_height < 100) {
            new_height = 100;
            if (edge == HIT_RESIZE_TOP || edge == HIT_RESIZE_TOP_LEFT || 
                edge == HIT_RESIZE_TOP_RIGHT) {
                new_y = cursor_state.grab_box.y + cursor_state.grab_box.height - 100;
            }
        }
        
        struct wlr_scene_node *node = cursor_state.toplevel->decor.tree ?
            &cursor_state.toplevel->decor.tree->node : 
            &cursor_state.toplevel->scene_tree->node;
        wlr_scene_node_set_position(node, new_x, new_y);
        wlr_xdg_toplevel_set_size(cursor_state.toplevel->xdg_toplevel, 
                                   new_width, new_height);
        
        wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
        return;
    }
    
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x,
                    event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

void cursor_motion_abs(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, cursor_motion_abs);
  struct wlr_pointer_motion_absolute_event *event = data;
  wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x,
                           event->y);
  process_cursor_motion(server, event->time_msec);
}

void cursor_axis(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, cursor_axis);
  struct wlr_pointer_axis_event *event = data;
  wlr_seat_pointer_notify_axis(
      server->seat, event->time_msec, event->orientation, event->delta,
      event->delta_discrete, event->source, event->relative_direction);
}

void cursor_frame(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, cursor_frame);
  wlr_seat_pointer_notify_frame(server->seat);
}

void request_cursor(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, request_cursor);
  struct wlr_seat_pointer_request_set_cursor_event *event = data;
  struct wlr_seat_client *focused = server->seat->pointer_state.focused_client;
  if (focused == event->seat_client) {
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x,
                           event->hotspot_y);
  }
}

#include "config.h"

static void keyboard_key(struct wl_listener *listener, void *data) {
  struct keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_keyboard_key_event *event = data;

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms =
      xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

  uint32_t mods = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
  bool handled = false;

  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    for (int i = 0; i < nsyms && !handled; i++) {
      for (int k = 0; k < config.keybind_count; k++) {
        struct keybind *kb = &config.keybinds[k];
        if (kb->modifiers == mods && kb->keysym == syms[i]) {
          handled = handle_keybind(keyboard->server, kb->action, kb->arg);
          break;
        }
      }
    }
  }

  if (!handled) {
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    wlr_seat_keyboard_notify_key(keyboard->server->seat, event->time_msec,
                                 event->keycode, event->state);
  }
}

static void keyboard_modifiers(struct wl_listener *listener, void *data) {
  struct keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                                     &keyboard->wlr_keyboard->modifiers);
}

static void keyboard_destroy(struct wl_listener *listener, void *data) {
  struct keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
  free(keyboard);
}

void new_input(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, new_input);
  struct wlr_input_device *device = data;

  if (device->type == WLR_INPUT_DEVICE_KEYBOARD) {
    struct keyboard *keyboard = calloc(1, sizeof(*keyboard));
    keyboard->server = server;
    keyboard->wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap =
        xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    wlr_keyboard_set_keymap(keyboard->wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);

    wlr_keyboard_set_repeat_info(keyboard->wlr_keyboard, 25, 600);

    keyboard->key.notify = keyboard_key;
    wl_signal_add(&keyboard->wlr_keyboard->events.key, &keyboard->key);

    keyboard->modifiers.notify = keyboard_modifiers;
    wl_signal_add(&keyboard->wlr_keyboard->events.modifiers,
                  &keyboard->modifiers);

    keyboard->destroy.notify = keyboard_destroy;
    wl_signal_add(&device->events.destroy, &keyboard->destroy);

    wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
    wl_list_insert(&server->keyboards, &keyboard->link);
  } else if (device->type == WLR_INPUT_DEVICE_POINTER) {
    wlr_cursor_attach_input_device(server->cursor, device);
  }

  uint32_t caps = WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;
  wlr_seat_set_capabilities(server->seat, caps);
}

/* Popup handler */
void server_new_xdg_popup(struct wl_listener *listener, void *data) {
  struct wlr_xdg_popup *popup = data;
  struct wlr_xdg_surface *parent =
      wlr_xdg_surface_try_from_wlr_surface(popup->parent);
  if (parent) {
    struct wlr_scene_tree *parent_tree = parent->data;
    popup->base->data = wlr_scene_xdg_surface_create(parent_tree, popup->base);
  }
}

void server_new_xdg_decoration(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
  wlr_xdg_toplevel_decoration_v1_set_mode(
      decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

/* Utility */
struct toplevel *toplevel_at(struct server *server, double lx, double ly,
                             struct wlr_surface **surface, double *sx,
                             double *sy) {
  struct wlr_scene_node *node =
      wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return NULL;
  }
  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return NULL;
  }
  *surface = scene_surface->surface;

  struct wlr_scene_tree *tree = node->parent;
  while (tree && !tree->node.data) {
    tree = tree->node.parent;
  }
  if (!tree) return NULL;
  
  /* Check it's a toplevel, not a layer surface */
  struct toplevel *toplevel;
  wl_list_for_each(toplevel, &server->toplevels, link) {
      if (toplevel->scene_tree == tree || 
          (toplevel->decor.tree && toplevel->decor.tree == tree)) {
          return toplevel;
      }
  }
  
  return NULL;
}
