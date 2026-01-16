#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
//#include "config.h"

// Forward declaration (defined in config.h)
enum keybind_action;
/* Output handlers */
static void output_frame(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, frame);
  wlr_scene_output_commit(output->scene_output, NULL);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(output->scene_output, &now);
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

/* Cursor handlers */
static void process_cursor_motion(struct server *server, uint32_t time) {
  double sx, sy;
  struct wlr_surface *surface = NULL;
  struct toplevel *toplevel = toplevel_at(
      server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (!toplevel) {
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    wlr_seat_pointer_clear_focus(server->seat);
  } else {
    wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
  }
}

void cursor_motion(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, cursor_motion);
  struct wlr_pointer_motion_event *event = data;
  if (cursor_state.mode == CURSOR_MOVE) {
    wlr_scene_node_set_position(&cursor_state.toplevel->scene_tree->node,
                                server->cursor->x - cursor_state.grab_x,
                                server->cursor->y - cursor_state.grab_y);
    return;
  }

  if (cursor_state.mode == CURSOR_RESIZE) {
    double dx = server->cursor->x - cursor_state.grab_x;
    double dy = server->cursor->y - cursor_state.grab_y;
    int new_width = cursor_state.grab_box.width + dx;
    int new_height = cursor_state.grab_box.height + dy;
    wlr_xdg_toplevel_set_size(cursor_state.toplevel->xdg_toplevel,
                              new_width > 50 ? new_width : 50,
                              new_height > 50 ? new_height : 50);
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
    // DEBUG
    printf("KEY: mods=0x%x keysym=0x%x, checking %d keybinds\n", 
           mods, nsyms > 0 ? syms[0] : 0, config.keybind_count);
    
    for (int i = 0; i < nsyms && !handled; i++) {
      for (int k = 0; k < config.keybind_count; k++) {
        struct keybind *kb = &config.keybinds[k];
        if (kb->modifiers == mods && kb->keysym == syms[i]) {
          printf("MATCH: action=%d arg='%s'\n", kb->action, kb->arg);
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

// core.c - layer surface handler
static void layer_surface_map(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, map);
  wlr_scene_node_set_enabled(&layer->scene_tree->node, true);
}

static void layer_surface_unmap(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, unmap);
  wlr_scene_node_set_enabled(&layer->scene_tree->node, false);
}

static void layer_surface_destroy(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, destroy);
  wl_list_remove(&layer->map.link);
  wl_list_remove(&layer->unmap.link);
  wl_list_remove(&layer->destroy.link);
  free(layer);
}
void server_new_layer_surface(struct wl_listener *listener, void *data) {
  struct server *server = wl_container_of(listener, server, new_layer_surface);
  struct wlr_layer_surface_v1 *wlr_layer = data;

  struct layer_surface *layer = calloc(1, sizeof(*layer));
  layer->server = server;
  layer->wlr_layer = wlr_layer;

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

  struct wlr_scene_layer_surface_v1 *scene_layer =
      wlr_scene_layer_surface_v1_create(parent, wlr_layer);
  layer->scene_tree = scene_layer->tree;
  wlr_layer->data = layer;

  layer->map.notify = layer_surface_map;
  wl_signal_add(&wlr_layer->surface->events.map, &layer->map);

  layer->unmap.notify = layer_surface_unmap;
  wl_signal_add(&wlr_layer->surface->events.unmap, &layer->unmap);

  layer->destroy.notify = layer_surface_destroy;
  wl_signal_add(&wlr_layer->events.destroy, &layer->destroy);

  struct wlr_output *output = wlr_layer->output;
  if (!output) {
    struct output *o;
    wl_list_for_each(o, &server->outputs, link) {
      output = o->wlr_output;
      break;
    }
  }
  if (output) {
    wlr_layer_surface_v1_configure(wlr_layer, output->width, output->height);
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
static void xdg_decoration_commit(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
  wlr_xdg_toplevel_decoration_v1_set_mode(
      decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
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
  return tree ? tree->node.data : NULL;
}
