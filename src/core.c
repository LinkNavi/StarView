#include "core.h"
#include "background.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "core.h"
#include <stdlib.h>
#include <wlr/xwayland.h>
#include <wlr/xwayland/shell.h>
#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_output_management_v1.h>
// Optional protocols - comment out if not available:
// #include <wlr/types/wlr_ext_session_lock_v1.h>
// #include <wlr/types/wlr_output_power_management_v1.h>

/*
 * XWayland Integration
 * Allows running X11 applications (Firefox, Steam, etc.)
 */

struct xwayland_surface {
  struct wl_list link;
  struct server *server;
  struct wlr_xwayland_surface *xwayland_surface;
  struct wlr_scene_tree *scene_tree;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener surface_map;  // For listening to surface->events.map
  struct wl_listener destroy;
  struct wl_listener request_configure;
  struct wl_listener request_fullscreen;
  struct wl_listener request_minimize;
  struct wl_listener request_activate;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener set_title;
  struct wl_listener set_class;
  struct wl_listener set_parent;
  struct wl_listener set_hints;
  struct wl_listener set_window_type;
  struct wl_listener set_override_redirect;

  // Window state tracking
  bool override_redirect;
  bool is_popup;       // Menus, tooltips, etc.
  bool is_dialog;      // Transient dialogs
  bool is_splash;      // Splash screens
  struct xwayland_surface *parent;
};

// Forward declarations
static void xwayland_surface_map(struct wl_listener *listener, void *data);
static void xwayland_surface_unmap(struct wl_listener *listener, void *data);

static void xwayland_surface_associate(struct wl_listener *listener, void *data) {
  struct xwayland_surface *xsurface = wl_container_of(listener, xsurface, map);
  (void)data;

  printf("XWayland surface associated\n");
  
  // Now that surface is available, listen to its map/unmap events
  if (xsurface->xwayland_surface->surface) {
    xsurface->surface_map.notify = xwayland_surface_map;
    wl_signal_add(&xsurface->xwayland_surface->surface->events.map, &xsurface->surface_map);
    
    xsurface->unmap.notify = xwayland_surface_unmap;
    wl_signal_add(&xsurface->xwayland_surface->surface->events.unmap, &xsurface->unmap);
    
    // If already mapped, call map handler immediately
    if (xsurface->xwayland_surface->surface->mapped) {
      xwayland_surface_map(&xsurface->surface_map, NULL);
    }
  }
}

static void xwayland_surface_dissociate(struct wl_listener *listener, void *data) {
  struct xwayland_surface *xsurface = wl_container_of(listener, xsurface, unmap);
  (void)data;
  
  printf("XWayland surface dissociated\n");
  
  if (xsurface->scene_tree) {
    wlr_scene_node_destroy(&xsurface->scene_tree->node);
    xsurface->scene_tree = NULL;
  }
}

static void xwayland_surface_map(struct wl_listener *listener, void *data) {
  struct xwayland_surface *xsurface = wl_container_of(listener, xsurface, surface_map);
  (void)data;

  if (!xsurface->xwayland_surface->surface) {
    fprintf(stderr, "XWayland surface has no wlr_surface\n");
    return;
  }

  // Determine the appropriate layer based on window type
  struct wlr_scene_tree *parent_layer;

  if (xsurface->override_redirect) {
    // Override-redirect windows (menus, tooltips, popups) go on overlay
    parent_layer = xsurface->server->layer_overlay;
  } else if (xsurface->is_popup || xsurface->is_splash) {
    // Popups and splash screens also go above normal windows
    parent_layer = xsurface->server->layer_top;
  } else {
    // Normal X11 windows
    parent_layer = xsurface->server->layer_windows;
  }

  // Create scene node for X11 window
  xsurface->scene_tree = wlr_scene_subsurface_tree_create(
      parent_layer, xsurface->xwayland_surface->surface);

  if (!xsurface->scene_tree) {
    fprintf(stderr, "Failed to create scene tree for XWayland surface\n");
    return;
  }

  xsurface->scene_tree->node.data = xsurface;

  // Position window
  wlr_scene_node_set_position(&xsurface->scene_tree->node,
                              xsurface->xwayland_surface->x,
                              xsurface->xwayland_surface->y);

  // For dialogs with a parent, position relative to parent if not already positioned
  if (xsurface->parent && xsurface->parent->scene_tree) {
    struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;
    struct wlr_xwayland_surface *parent_xs = xsurface->parent->xwayland_surface;

    // If position is 0,0, center on parent
    if (xs->x == 0 && xs->y == 0) {
      int cx = parent_xs->x + (parent_xs->width - xs->width) / 2;
      int cy = parent_xs->y + (parent_xs->height - xs->height) / 2;
      wlr_scene_node_set_position(&xsurface->scene_tree->node, cx, cy);
      wlr_xwayland_surface_configure(xs, cx, cy, xs->width, xs->height);
    }

    // Raise dialog above parent
    wlr_scene_node_raise_to_top(&xsurface->scene_tree->node);
  }

  // Focus non-OR windows automatically if they're not popups
  if (!xsurface->override_redirect && !xsurface->is_popup) {
    wlr_xwayland_surface_activate(xsurface->xwayland_surface, true);
    if (xsurface->server->seat && xsurface->xwayland_surface->surface) {
      wlr_seat_keyboard_notify_enter(xsurface->server->seat,
                                      xsurface->xwayland_surface->surface,
                                      NULL, 0, NULL);
    }
  }

  printf("XWayland surface mapped: %s (%s) at %d,%d [OR=%d popup=%d dialog=%d]\n",
         xsurface->xwayland_surface->title ? xsurface->xwayland_surface->title : "null",
         xsurface->xwayland_surface->class ? xsurface->xwayland_surface->class : "null",
         xsurface->xwayland_surface->x,
         xsurface->xwayland_surface->y,
         xsurface->override_redirect,
         xsurface->is_popup,
         xsurface->is_dialog);
}

static void xwayland_surface_unmap(struct wl_listener *listener, void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, unmap);
  (void)data;

  printf("XWayland surface unmapping: %s\n", 
         xsurface->xwayland_surface->title ? xsurface->xwayland_surface->title : "null");

  if (xsurface->scene_tree) {
    wlr_scene_node_destroy(&xsurface->scene_tree->node);
    xsurface->scene_tree = NULL;
  }
}

static void xwayland_surface_destroy(struct wl_listener *listener, void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, destroy);
  (void)data;

  printf("XWayland surface destroying: %p\n", (void*)xsurface->xwayland_surface);

  if (xsurface->scene_tree) {
    wlr_scene_node_destroy(&xsurface->scene_tree->node);
    xsurface->scene_tree = NULL;
  }

  wl_list_remove(&xsurface->map.link);
  wl_list_remove(&xsurface->unmap.link);
  wl_list_remove(&xsurface->surface_map.link);
  wl_list_remove(&xsurface->destroy.link);
  wl_list_remove(&xsurface->request_configure.link);
  wl_list_remove(&xsurface->request_fullscreen.link);
  wl_list_remove(&xsurface->request_minimize.link);
  wl_list_remove(&xsurface->request_activate.link);
  wl_list_remove(&xsurface->request_move.link);
  wl_list_remove(&xsurface->request_resize.link);
  wl_list_remove(&xsurface->set_title.link);
  wl_list_remove(&xsurface->set_class.link);
  wl_list_remove(&xsurface->set_parent.link);
  wl_list_remove(&xsurface->set_hints.link);
  wl_list_remove(&xsurface->set_window_type.link);
  wl_list_remove(&xsurface->set_override_redirect.link);
  wl_list_remove(&xsurface->link);

  free(xsurface);
}

static void xwayland_surface_request_configure(struct wl_listener *listener,
                                               void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_configure);
  struct wlr_xwayland_surface_configure_event *event = data;

  wlr_xwayland_surface_configure(xsurface->xwayland_surface, event->x, event->y,
                                 event->width, event->height);

  if (xsurface->scene_tree) {
    wlr_scene_node_set_position(&xsurface->scene_tree->node, event->x,
                                event->y);
  }
}

static void xwayland_surface_request_fullscreen(struct wl_listener *listener,
                                                void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_fullscreen);

  bool fullscreen = xsurface->xwayland_surface->fullscreen;

  if (fullscreen) {
    struct output *output;
    wl_list_for_each(output, &xsurface->server->outputs, link) {
      wlr_xwayland_surface_configure(xsurface->xwayland_surface, 0, 0,
                                     output->wlr_output->width,
                                     output->wlr_output->height);
      break;
    }
  }
}

static void xwayland_surface_request_minimize(struct wl_listener *listener,
                                              void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_minimize);

  if (xsurface->scene_tree) {
    bool minimize = xsurface->xwayland_surface->minimized;
    wlr_scene_node_set_enabled(&xsurface->scene_tree->node, !minimize);
  }
}

static void xwayland_surface_request_activate(struct wl_listener *listener,
                                              void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_activate);
  struct wlr_xwayland_surface *xwayland_surface = xsurface->xwayland_surface;

  // Safety checks
  if (!xwayland_surface->surface) {
    return;
  }
  
  if (!xwayland_surface->surface->mapped) {
    return;
  }

  // Only activate if the surface is mapped and valid
  wlr_xwayland_surface_activate(xwayland_surface, true);
  
  // Focus keyboard to this surface
  if (xsurface->server->seat) {
    wlr_seat_keyboard_notify_enter(xsurface->server->seat, 
                                    xwayland_surface->surface,
                                    NULL, 0, NULL);
  }
}

static void xwayland_surface_set_title(struct wl_listener *listener,
                                       void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_title);
  (void)data;

  if (xsurface->xwayland_surface->title) {
    printf("XWayland title changed: %s\n", xsurface->xwayland_surface->title);
  }
}

static void xwayland_surface_set_class(struct wl_listener *listener,
                                       void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_class);
  (void)data;

  if (xsurface->xwayland_surface->class) {
    printf("XWayland class changed: %s\n", xsurface->xwayland_surface->class);
  }
}

static void xwayland_surface_set_parent(struct wl_listener *listener,
                                        void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_parent);
  (void)data;

  struct wlr_xwayland_surface *parent_xsurface =
      xsurface->xwayland_surface->parent;

  if (parent_xsurface) {
    // Find our parent xwayland_surface wrapper
    struct xwayland_surface *parent;
    wl_list_for_each(parent, &xsurface->server->xwayland_surfaces, link) {
      if (parent->xwayland_surface == parent_xsurface) {
        xsurface->parent = parent;
        xsurface->is_dialog = true;
        printf("XWayland surface has parent: %s\n",
               parent_xsurface->title ? parent_xsurface->title : "untitled");
        break;
      }
    }
  } else {
    xsurface->parent = NULL;
    xsurface->is_dialog = false;
  }
}

static void xwayland_surface_set_hints(struct wl_listener *listener,
                                       void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_hints);
  (void)data;

  struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;

  if (xs->hints) {
    // Handle urgency hint
    if (xs->hints->flags & XCB_ICCCM_WM_HINT_X_URGENCY) {
      printf("XWayland surface urgent: %s\n",
             xs->title ? xs->title : "untitled");
    }

    // Handle input hint (whether app accepts focus)
    if (xs->hints->flags & XCB_ICCCM_WM_HINT_INPUT) {
      printf("XWayland surface input hint: %d\n", xs->hints->input);
    }
  }
}

// Window type detection helper
static void update_window_type(struct xwayland_surface *xsurface) {
  struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;

  xsurface->is_popup = false;
  xsurface->is_dialog = false;
  xsurface->is_splash = false;

  if (!xs->window_type) {
    return;
  }

  // Check window types (NET_WM_WINDOW_TYPE)
  for (size_t i = 0; i < xs->window_type_len; i++) {
    xcb_atom_t type = xs->window_type[i];

    // Common window type atoms
    // Note: These are typically compared against cached atoms from xwayland
    // For now, we use heuristics based on other properties
  }

  // Use heuristics for window type detection
  if (xs->parent) {
    xsurface->is_dialog = true;
  }

  // Modal windows are dialogs
  if (xs->modal) {
    xsurface->is_dialog = true;
  }
}

static void xwayland_surface_set_window_type(struct wl_listener *listener,
                                              void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_window_type);
  (void)data;

  update_window_type(xsurface);
  printf("XWayland window type updated: popup=%d dialog=%d splash=%d\n",
         xsurface->is_popup, xsurface->is_dialog, xsurface->is_splash);
}

static void xwayland_surface_set_override_redirect(struct wl_listener *listener,
                                                    void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, set_override_redirect);
  (void)data;

  bool was_or = xsurface->override_redirect;
  xsurface->override_redirect = xsurface->xwayland_surface->override_redirect;

  if (was_or != xsurface->override_redirect) {
    printf("XWayland override-redirect changed: %d -> %d\n",
           was_or, xsurface->override_redirect);

    // Reposition override-redirect windows (menus, tooltips) to overlay layer
    if (xsurface->scene_tree && xsurface->override_redirect) {
      wlr_scene_node_reparent(&xsurface->scene_tree->node,
                               xsurface->server->layer_overlay);
    } else if (xsurface->scene_tree) {
      wlr_scene_node_reparent(&xsurface->scene_tree->node,
                               xsurface->server->layer_windows);
    }
  }
}

static void xwayland_surface_request_move(struct wl_listener *listener,
                                           void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_move);
  (void)data;

  // Don't allow OR windows to initiate move
  if (xsurface->override_redirect) {
    return;
  }

  // Start interactive move
  printf("XWayland surface requested move: %s\n",
         xsurface->xwayland_surface->title ?
         xsurface->xwayland_surface->title : "untitled");

  // Set cursor mode to move and track this surface
  if (xsurface->server->seat) {
    cursor_state.mode = CURSOR_MOVE;
    cursor_state.toplevel = NULL;  // We need a different tracking for xwayland
    cursor_state.grab_x = xsurface->server->cursor->x -
                          xsurface->xwayland_surface->x;
    cursor_state.grab_y = xsurface->server->cursor->y -
                          xsurface->xwayland_surface->y;
  }
}

static void xwayland_surface_request_resize(struct wl_listener *listener,
                                             void *data) {
  struct xwayland_surface *xsurface =
      wl_container_of(listener, xsurface, request_resize);
  struct wlr_xwayland_resize_event *event = data;

  // Don't allow OR windows to initiate resize
  if (xsurface->override_redirect) {
    return;
  }

  printf("XWayland surface requested resize: %s (edges: %d)\n",
         xsurface->xwayland_surface->title ?
         xsurface->xwayland_surface->title : "untitled",
         event->edges);

  // Set cursor mode to resize
  cursor_state.mode = CURSOR_RESIZE;
  cursor_state.toplevel = NULL;
  cursor_state.resize_edges = event->edges;
  cursor_state.grab_x = xsurface->server->cursor->x;
  cursor_state.grab_y = xsurface->server->cursor->y;
  cursor_state.grab_box.x = xsurface->xwayland_surface->x;
  cursor_state.grab_box.y = xsurface->xwayland_surface->y;
  cursor_state.grab_box.width = xsurface->xwayland_surface->width;
  cursor_state.grab_box.height = xsurface->xwayland_surface->height;
}

static void server_new_xwayland_surface(struct wl_listener *listener,
                                        void *data) {
  struct server *server =
      wl_container_of(listener, server, new_xwayland_surface);
  struct wlr_xwayland_surface *xwayland_surface = data;

  struct xwayland_surface *xsurface = calloc(1, sizeof(*xsurface));
  if (!xsurface) {
    return;
  }

  xsurface->server = server;
  xsurface->xwayland_surface = xwayland_surface;
  xsurface->override_redirect = xwayland_surface->override_redirect;

  // Use associate/dissociate events which exist in wlroots 0.18
  xsurface->map.notify = xwayland_surface_associate;
  wl_signal_add(&xwayland_surface->events.associate, &xsurface->map);

  xsurface->unmap.notify = xwayland_surface_dissociate;
  wl_signal_add(&xwayland_surface->events.dissociate, &xsurface->unmap);

  xsurface->destroy.notify = xwayland_surface_destroy;
  wl_signal_add(&xwayland_surface->events.destroy, &xsurface->destroy);

  xsurface->request_configure.notify = xwayland_surface_request_configure;
  wl_signal_add(&xwayland_surface->events.request_configure,
                &xsurface->request_configure);

  xsurface->request_fullscreen.notify = xwayland_surface_request_fullscreen;
  wl_signal_add(&xwayland_surface->events.request_fullscreen,
                &xsurface->request_fullscreen);

  xsurface->request_minimize.notify = xwayland_surface_request_minimize;
  wl_signal_add(&xwayland_surface->events.request_minimize,
                &xsurface->request_minimize);

  xsurface->request_activate.notify = xwayland_surface_request_activate;
  wl_signal_add(&xwayland_surface->events.request_activate,
                &xsurface->request_activate);

  // New: request_move and request_resize
  xsurface->request_move.notify = xwayland_surface_request_move;
  wl_signal_add(&xwayland_surface->events.request_move,
                &xsurface->request_move);

  xsurface->request_resize.notify = xwayland_surface_request_resize;
  wl_signal_add(&xwayland_surface->events.request_resize,
                &xsurface->request_resize);

  xsurface->set_title.notify = xwayland_surface_set_title;
  wl_signal_add(&xwayland_surface->events.set_title, &xsurface->set_title);

  xsurface->set_class.notify = xwayland_surface_set_class;
  wl_signal_add(&xwayland_surface->events.set_class, &xsurface->set_class);

  // New: parent, hints, window type, and override redirect events
  xsurface->set_parent.notify = xwayland_surface_set_parent;
  wl_signal_add(&xwayland_surface->events.set_parent, &xsurface->set_parent);

  xsurface->set_hints.notify = xwayland_surface_set_hints;
  wl_signal_add(&xwayland_surface->events.set_hints, &xsurface->set_hints);

  xsurface->set_window_type.notify = xwayland_surface_set_window_type;
  wl_signal_add(&xwayland_surface->events.set_window_type,
                &xsurface->set_window_type);

  xsurface->set_override_redirect.notify = xwayland_surface_set_override_redirect;
  wl_signal_add(&xwayland_surface->events.set_override_redirect,
                &xsurface->set_override_redirect);

  wl_list_insert(&server->xwayland_surfaces, &xsurface->link);

  // Initial window type detection
  update_window_type(xsurface);

  printf("New XWayland surface created: %p (OR=%d)\n",
         (void*)xwayland_surface, xsurface->override_redirect);
}

// XWayland ready callback - called when Xwayland server is ready
static void xwayland_ready(struct wl_listener *listener, void *data) {
  struct server *server =
      wl_container_of(listener, server, xwayland_ready);
  (void)data;

  // Set cursor theme for XWayland
  if (server->cursor_mgr && server->xwayland) {
    wlr_xwayland_set_cursor(server->xwayland,
        server->cursor_mgr->theme->cursors[0]->images[0]->buffer,
        server->cursor_mgr->theme->cursors[0]->images[0]->width * 4,
        server->cursor_mgr->theme->cursors[0]->images[0]->width,
        server->cursor_mgr->theme->cursors[0]->images[0]->height,
        server->cursor_mgr->theme->cursors[0]->images[0]->hotspot_x,
        server->cursor_mgr->theme->cursors[0]->images[0]->hotspot_y);
  }

  printf("XWayland server is ready\n");
}

int xwayland_init(struct server *server) {
  wl_list_init(&server->xwayland_surfaces);

  server->xwayland =
      wlr_xwayland_create(server->display, server->compositor, true);

  if (!server->xwayland) {
    fprintf(stderr, "Failed to create XWayland server\n");
    return -1;
  }

  server->new_xwayland_surface.notify = server_new_xwayland_surface;
  wl_signal_add(&server->xwayland->events.new_surface,
                &server->new_xwayland_surface);

  // Listen for XWayland ready event
  server->xwayland_ready.notify = xwayland_ready;
  wl_signal_add(&server->xwayland->events.ready, &server->xwayland_ready);

  // IMPORTANT: Assign the seat to XWayland for selection/clipboard support
  wlr_xwayland_set_seat(server->xwayland, server->seat);

  // Set environment variables
  setenv("DISPLAY", server->xwayland->display_name, true);

  // Enable XIM for input method support in X11 applications
  // This allows apps like Firefox, Chrome, etc. to use IME
  setenv("XMODIFIERS", "@im=ibus", false);  // Don't override if already set
  setenv("GTK_IM_MODULE", "ibus", false);
  setenv("QT_IM_MODULE", "ibus", false);

  // HiDPI scaling: Set global scale for XWayland
  // Can be configured per-output later
  struct output *output;
  float max_scale = 1.0f;
  wl_list_for_each(output, &server->outputs, link) {
    if (output->wlr_output->scale > max_scale) {
      max_scale = output->wlr_output->scale;
    }
  }

  // Set GDK_SCALE for GTK apps on XWayland
  if (max_scale > 1.0f) {
    char scale_str[16];
    snprintf(scale_str, sizeof(scale_str), "%d", (int)max_scale);
    setenv("GDK_SCALE", scale_str, true);
    setenv("GDK_DPI_SCALE", "1", true);  // Prevent double scaling
  }

  printf("XWayland initialized on DISPLAY=%s (scale=%.1f)\n",
         server->xwayland->display_name, max_scale);

  return 0;
}

void xwayland_finish(struct server *server) {
  if (server->xwayland) {
    wl_list_remove(&server->new_xwayland_surface.link);
    wl_list_remove(&server->xwayland_ready.link);
    wlr_xwayland_destroy(server->xwayland);
    server->xwayland = NULL;
  }
}

/*
 * XWayland Stacking Management
 * Helper functions for managing X11 window stacking order
 */

// Raise an XWayland surface above its siblings
void xwayland_surface_raise(struct xwayland_surface *xsurface) {
  if (!xsurface || !xsurface->scene_tree) {
    return;
  }

  wlr_scene_node_raise_to_top(&xsurface->scene_tree->node);

  // Also raise any child surfaces (transients, dialogs)
  struct xwayland_surface *child;
  wl_list_for_each(child, &xsurface->server->xwayland_surfaces, link) {
    if (child->parent == xsurface && child->scene_tree) {
      wlr_scene_node_raise_to_top(&child->scene_tree->node);
    }
  }
}

// Lower an XWayland surface below its siblings
void xwayland_surface_lower(struct xwayland_surface *xsurface) {
  if (!xsurface || !xsurface->scene_tree) {
    return;
  }

  // First lower children
  struct xwayland_surface *child;
  wl_list_for_each(child, &xsurface->server->xwayland_surfaces, link) {
    if (child->parent == xsurface && child->scene_tree) {
      wlr_scene_node_lower_to_bottom(&child->scene_tree->node);
    }
  }

  wlr_scene_node_lower_to_bottom(&xsurface->scene_tree->node);
}

// Restack a surface relative to another
void xwayland_surface_restack(struct xwayland_surface *xsurface,
                               struct xwayland_surface *sibling,
                               bool above) {
  if (!xsurface || !xsurface->scene_tree || !sibling || !sibling->scene_tree) {
    return;
  }

  if (above) {
    wlr_scene_node_place_above(&xsurface->scene_tree->node,
                                &sibling->scene_tree->node);
  } else {
    wlr_scene_node_place_below(&xsurface->scene_tree->node,
                                &sibling->scene_tree->node);
  }
}

// Find XWayland surface at coordinates
struct xwayland_surface *xwayland_surface_at(struct server *server,
                                              double lx, double ly,
                                              struct wlr_surface **surface,
                                              double *sx, double *sy) {
  struct xwayland_surface *xsurface;
  wl_list_for_each(xsurface, &server->xwayland_surfaces, link) {
    if (!xsurface->scene_tree) {
      continue;
    }

    struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;
    if (!xs->surface || !xs->surface->mapped) {
      continue;
    }

    // Check bounds
    if (lx >= xs->x && lx < xs->x + xs->width &&
        ly >= xs->y && ly < xs->y + xs->height) {
      *sx = lx - xs->x;
      *sy = ly - xs->y;
      *surface = xs->surface;
      return xsurface;
    }
  }

  return NULL;
}

// Focus an XWayland surface
void xwayland_surface_focus(struct xwayland_surface *xsurface) {
  if (!xsurface || !xsurface->xwayland_surface->surface) {
    return;
  }

  struct server *server = xsurface->server;
  struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;

  // Don't focus override-redirect windows
  if (xsurface->override_redirect) {
    return;
  }

  // Activate the surface
  wlr_xwayland_surface_activate(xs, true);

  // Send keyboard enter
  if (server->seat && xs->surface->mapped) {
    struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
    if (keyboard) {
      wlr_seat_keyboard_notify_enter(server->seat, xs->surface,
                                      keyboard->keycodes,
                                      keyboard->num_keycodes,
                                      &keyboard->modifiers);
    }
  }

  // Raise to top with transients
  xwayland_surface_raise(xsurface);
}

// Send configure event respecting constraints
void xwayland_surface_configure(struct xwayland_surface *xsurface,
                                 int x, int y, int width, int height) {
  if (!xsurface) {
    return;
  }

  struct wlr_xwayland_surface *xs = xsurface->xwayland_surface;

  // Respect size hints if available
  if (xs->size_hints) {
    // Minimum size
    if (xs->size_hints->min_width > 0 && width < xs->size_hints->min_width) {
      width = xs->size_hints->min_width;
    }
    if (xs->size_hints->min_height > 0 && height < xs->size_hints->min_height) {
      height = xs->size_hints->min_height;
    }

    // Maximum size
    if (xs->size_hints->max_width > 0 && width > xs->size_hints->max_width) {
      width = xs->size_hints->max_width;
    }
    if (xs->size_hints->max_height > 0 && height > xs->size_hints->max_height) {
      height = xs->size_hints->max_height;
    }
  }

  wlr_xwayland_surface_configure(xs, x, y, width, height);

  if (xsurface->scene_tree) {
    wlr_scene_node_set_position(&xsurface->scene_tree->node, x, y);
  }
}

/* Session lock support - DISABLED (protocol not available) */
int session_lock_init(struct server *server) {
    (void)server;
    printf("Session lock support disabled (protocol not available)\n");
    printf("Install ext-session-lock-v1 protocol to enable swaylock support\n");
    return 0;
}

/* Initialize idle and locking support */
int idle_init(struct server *server) {
    // New idle notify protocol
    server->idle_notifier = wlr_idle_notifier_v1_create(server->display);
    if (!server->idle_notifier) {
        fprintf(stderr, "Failed to create idle notifier\n");
        return -1;
    }
    
    // Idle inhibit (prevents screen blanking during videos, games, etc.)
    server->idle_inhibit = 
        wlr_idle_inhibit_v1_create(server->display);
    if (!server->idle_inhibit) {
        fprintf(stderr, "Failed to create idle inhibit manager\n");
        return -1;
    }
    
    printf("Idle management initialized\n");
    return 0;
}

int clipboard_init(struct server *server) {
    server->primary_selection = 
        wlr_primary_selection_v1_device_manager_create(server->display);
    if (!server->primary_selection) {
        fprintf(stderr, "Failed to create primary selection manager\n");
        return -1;
    }
    
    server->data_control = 
        wlr_data_control_manager_v1_create(server->display);
    if (!server->data_control) {
        fprintf(stderr, "Failed to create data control manager\n");
        return -1;
    }
    
    printf("Clipboard support initialized\n");
    return 0;
}

/* Output handlers */
static void output_frame(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, frame);
  (void)data;
  wlr_scene_output_commit(output->scene_output, NULL);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(output->scene_output, &now);
}

static void layer_surface_map(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, map);
  (void)data;

  printf("Layer MAPPED: %s anchor=%d exclusive=%d size=%dx%d\n",
         layer->wlr_layer->namespace, layer->wlr_layer->current.anchor,
         layer->wlr_layer->current.exclusive_zone,
         layer->wlr_layer->surface->current.width,
         layer->wlr_layer->surface->current.height);

  wlr_surface_send_enter(layer->wlr_layer->surface, layer->wlr_layer->output);
}

static void layer_surface_unmap(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, unmap);
  (void)data;
  printf("Layer unmapped: %s\n", layer->wlr_layer->namespace);
}

static void layer_surface_commit(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, commit);
  (void)data;
  struct wlr_output *output = layer->wlr_layer->output;

  if (!output)
    return;

  struct wlr_box full_box = {
      .x = 0, .y = 0, .width = output->width, .height = output->height};

  wlr_scene_layer_surface_v1_configure(layer->scene_layer, &full_box,
                                       &full_box);
}

static void layer_surface_destroy(struct wl_listener *listener, void *data) {
  struct layer_surface *layer = wl_container_of(listener, layer, destroy);
  (void)data;
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

  if (output->background && config.background.enabled) {
    background_update(output->background, output->wlr_output->width,
                      output->wlr_output->height, &config.background);
  }
}

static void output_destroy(struct wl_listener *listener, void *data) {
  struct output *output = wl_container_of(listener, output, destroy);
  (void)data;

  if (output->background) {
    wlr_scene_node_destroy(&output->background->node);
  }

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

  if (config.background.enabled) {
    output->background =
        background_create(server->layer_bg, wlr_output->width,
                          wlr_output->height, &config.background);
  }
  wl_list_insert(&server->outputs, &output->link);
}

static struct layer_surface *layer_surface_at(struct server *server, double lx,
                                              double ly,
                                              struct wlr_surface **surface,
                                              double *sx, double *sy) {
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

  struct layer_surface *layer = layer_surface_at(
      server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (layer && surface) {
    wlr_seat_pointer_notify_enter(server->seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(server->seat, time, sx, sy);
    return;
  }

  struct toplevel *toplevel = toplevel_at(
      server, server->cursor->x, server->cursor->y, &surface, &sx, &sy);

  if (!toplevel) {
    wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
    wlr_seat_pointer_clear_focus(server->seat);
  } else {
    if (config.decor.enabled) {
      decor_update_hover(toplevel, server->cursor->x, server->cursor->y);

      enum decor_hit hit =
          decor_hit_test(toplevel, server->cursor->x, server->cursor->y);
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
  (void)data;
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
  (void)data;
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                                     &keyboard->wlr_keyboard->modifiers);
}

static void keyboard_destroy(struct wl_listener *listener, void *data) {
  struct keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
  (void)data;
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

void server_new_xdg_popup(struct wl_listener *listener, void *data) {
  (void)listener;
  struct wlr_xdg_popup *popup = data;
  struct wlr_xdg_surface *parent =
      wlr_xdg_surface_try_from_wlr_surface(popup->parent);
  if (parent) {
    struct wlr_scene_tree *parent_tree = parent->data;
    popup->base->data = wlr_scene_xdg_surface_create(parent_tree, popup->base);
  }
}

/* Multi-monitor support - BASIC (no DPMS) */
static void output_manager_apply(struct wl_listener *listener, void *data) {
    struct server *server = 
        wl_container_of(listener, server, output_manager_apply);
    struct wlr_output_configuration_v1 *config = data;
    
    bool ok = true;
    
    struct wlr_output_configuration_head_v1 *head;
    wl_list_for_each(head, &config->heads, link) {
        struct wlr_output *wlr_output = head->state.output;
        
        struct wlr_output_state state;
        wlr_output_state_init(&state);
        
        // Apply mode
        if (head->state.mode) {
            wlr_output_state_set_mode(&state, head->state.mode);
        } else if (head->state.custom_mode.width > 0) {
            wlr_output_state_set_custom_mode(&state,
                head->state.custom_mode.width,
                head->state.custom_mode.height,
                head->state.custom_mode.refresh);
        }
        
        // Apply scale
        if (head->state.scale > 0) {
            wlr_output_state_set_scale(&state, head->state.scale);
        }
        
        // Apply transform
        wlr_output_state_set_transform(&state, head->state.transform);
        
        // Enable/disable
        wlr_output_state_set_enabled(&state, head->state.enabled);
        
        if (!wlr_output_commit_state(wlr_output, &state)) {
            ok = false;
        }
        
        wlr_output_state_finish(&state);
    }
    
    if (ok) {
        wlr_output_configuration_v1_send_succeeded(config);
    } else {
        wlr_output_configuration_v1_send_failed(config);
    }
    
    wlr_output_configuration_v1_destroy(config);
    arrange_windows(server);
}

static void output_manager_test(struct wl_listener *listener, void *data) {
    struct wlr_output_configuration_v1 *config = data;
    
    bool ok = true;
    struct wlr_output_configuration_head_v1 *head;
    wl_list_for_each(head, &config->heads, link) {
        struct wlr_output *wlr_output = head->state.output;
        
        if (head->state.mode) {
            struct wlr_output_state state;
            wlr_output_state_init(&state);
            wlr_output_state_set_mode(&state, head->state.mode);
            
            if (!wlr_output_test_state(wlr_output, &state)) {
                ok = false;
                wlr_output_state_finish(&state);
                break;
            }
            wlr_output_state_finish(&state);
        }
    }
    
    if (ok) {
        wlr_output_configuration_v1_send_succeeded(config);
    } else {
        wlr_output_configuration_v1_send_failed(config);
    }
    
    wlr_output_configuration_v1_destroy(config);
}

int multimonitor_init(struct server *server) {
    server->output_manager = 
        wlr_output_manager_v1_create(server->display);
    if (!server->output_manager) {
        fprintf(stderr, "Failed to create output manager\n");
        return -1;
    }
    
    server->output_manager_apply.notify = output_manager_apply;
    wl_signal_add(&server->output_manager->events.apply,
                  &server->output_manager_apply);
    
    server->output_manager_test.notify = output_manager_test;
    wl_signal_add(&server->output_manager->events.test,
                  &server->output_manager_test);
    
    printf("Multi-monitor support initialized (basic mode)\n");
    printf("DPMS disabled - protocol not available\n");
    return 0;
}

/* IME support - BASIC (simplified) */
struct text_input {
    struct wl_list link;
    struct server *server;
    struct wlr_text_input_v3 *text_input;
    
    struct wl_listener enable;
    struct wl_listener disable;
    struct wl_listener commit;
    struct wl_listener destroy;
};

static void relay_send_im_state(struct server *server) {
    if (!server->input_method || !server->active_text_input) {
        return;
    }
    
    struct wlr_text_input_v3 *text_input = server->active_text_input;
    struct wlr_input_method_v2 *input_method = server->input_method;
    
    wlr_input_method_v2_send_surrounding_text(input_method,
        text_input->current.surrounding.text,
        text_input->current.surrounding.cursor,
        text_input->current.surrounding.anchor);
    
    wlr_input_method_v2_send_text_change_cause(input_method,
        text_input->current.text_change_cause);
    
    wlr_input_method_v2_send_content_type(input_method,
        text_input->current.content_type.hint,
        text_input->current.content_type.purpose);
    
    wlr_input_method_v2_send_done(input_method);
}

static void text_input_enable(struct wl_listener *listener, void *data) {
    struct text_input *ti = wl_container_of(listener, ti, enable);
    
    if (ti->server->active_text_input == ti->text_input) {
        return;
    }
    
    ti->server->active_text_input = ti->text_input;
    relay_send_im_state(ti->server);
}

static void text_input_disable(struct wl_listener *listener, void *data) {
    struct text_input *ti = wl_container_of(listener, ti, disable);
    
    if (ti->server->active_text_input != ti->text_input) {
        return;
    }
    
    ti->server->active_text_input = NULL;
    
    if (ti->server->input_method) {
        wlr_input_method_v2_send_deactivate(ti->server->input_method);
        wlr_input_method_v2_send_done(ti->server->input_method);
    }
}

static void text_input_commit(struct wl_listener *listener, void *data) {
    struct text_input *ti = wl_container_of(listener, ti, commit);
    
    if (ti->server->active_text_input != ti->text_input) {
        return;
    }
    
    relay_send_im_state(ti->server);
}

static void text_input_destroy(struct wl_listener *listener, void *data) {
    struct text_input *ti = wl_container_of(listener, ti, destroy);
    
    if (ti->server->active_text_input == ti->text_input) {
        ti->server->active_text_input = NULL;
    }
    
    wl_list_remove(&ti->enable.link);
    wl_list_remove(&ti->disable.link);
    wl_list_remove(&ti->commit.link);
    wl_list_remove(&ti->destroy.link);
    wl_list_remove(&ti->link);
    free(ti);
}

static void handle_new_text_input(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_text_input);
    struct wlr_text_input_v3 *wlr_text_input = data;
    
    struct text_input *ti = calloc(1, sizeof(*ti));
    if (!ti) {
        return;
    }
    
    ti->server = server;
    ti->text_input = wlr_text_input;
    
    ti->enable.notify = text_input_enable;
    wl_signal_add(&wlr_text_input->events.enable, &ti->enable);
    
    ti->disable.notify = text_input_disable;
    wl_signal_add(&wlr_text_input->events.disable, &ti->disable);
    
    ti->commit.notify = text_input_commit;
    wl_signal_add(&wlr_text_input->events.commit, &ti->commit);
    
    ti->destroy.notify = text_input_destroy;
    wl_signal_add(&wlr_text_input->events.destroy, &ti->destroy);
}

static void input_method_commit(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_input_method);
    
    if (!server->input_method || !server->active_text_input) {
        return;
    }
    
    struct wlr_input_method_v2 *im = server->input_method;
    struct wlr_text_input_v3 *ti = server->active_text_input;
    
    if (im->current.commit_text) {
        wlr_text_input_v3_send_commit_string(ti, im->current.commit_text);
    }
    
    if (im->current.preedit.text) {
        wlr_text_input_v3_send_preedit_string(ti,
            im->current.preedit.text,
            im->current.preedit.cursor_begin,
            im->current.preedit.cursor_end);
    }
    
    if (im->current.delete.before_length || 
        im->current.delete.after_length) {
        wlr_text_input_v3_send_delete_surrounding_text(ti,
            im->current.delete.before_length,
            im->current.delete.after_length);
    }
    
    wlr_text_input_v3_send_done(ti);
}

static void input_method_destroy(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_input_method);
    
    server->input_method = NULL;
    
    if (server->active_text_input) {
        struct wlr_text_input_v3 *ti = server->active_text_input;
        wlr_text_input_v3_send_leave(ti);
        server->active_text_input = NULL;
    }
}

static void handle_new_input_method(struct wl_listener *listener, void *data) {
    struct server *server = wl_container_of(listener, server, new_input_method);
    struct wlr_input_method_v2 *input_method = data;
    
    if (server->input_method) {
        wlr_input_method_v2_send_unavailable(input_method);
        return;
    }
    
    server->input_method = input_method;
    
    struct wl_listener *commit_listener = calloc(1, sizeof(*commit_listener));
    commit_listener->notify = input_method_commit;
    wl_signal_add(&input_method->events.commit, commit_listener);
    
    struct wl_listener *destroy_listener = calloc(1, sizeof(*destroy_listener));
    destroy_listener->notify = input_method_destroy;
    wl_signal_add(&input_method->events.destroy, destroy_listener);
    
    if (server->active_text_input) {
        wlr_input_method_v2_send_activate(input_method);
        relay_send_im_state(server);
    }
}

int ime_init(struct server *server) {
    server->text_input = wlr_text_input_manager_v3_create(server->display);
    if (!server->text_input) {
        fprintf(stderr, "Failed to create text input manager\n");
        return -1;
    }
    
    server->new_text_input.notify = handle_new_text_input;
    wl_signal_add(&server->text_input->events.text_input,
                  &server->new_text_input);
    
    server->input_method_mgr = 
        wlr_input_method_manager_v2_create(server->display);
    if (!server->input_method_mgr) {
        fprintf(stderr, "Failed to create input method manager\n");
        return -1;
    }
    
    server->new_input_method.notify = handle_new_input_method;
    wl_signal_add(&server->input_method_mgr->events.input_method,
                  &server->new_input_method);
    
    server->input_method = NULL;
    server->active_text_input = NULL;
    
    printf("IME support initialized\n");
    return 0;
}

void server_new_xdg_decoration(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel_decoration_v1 *decoration = data;
  wlr_xdg_toplevel_decoration_v1_set_mode(
      decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

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
  if (!tree)
    return NULL;

  struct toplevel *toplevel;
  wl_list_for_each(toplevel, &server->toplevels, link) {
    if (toplevel->scene_tree == tree ||
        (toplevel->decor.tree && toplevel->decor.tree == tree)) {
      return toplevel;
    }
  }

  return NULL;
}
