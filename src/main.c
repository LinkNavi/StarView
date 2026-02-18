#define _POSIX_C_SOURCE 200809L

#include "background.h"
#include "config.h"
#include "core.h"
#include "gesture.h"
#include "ipc.h"
#include "titlebar_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>

#include "decor_visual.h"
#include "protocols.h"

int main(void) {
  wlr_log_init(WLR_DEBUG, NULL);

  struct server server = {0};
  wl_list_init(&server.outputs);
  wl_list_init(&server.toplevels);
  wl_list_init(&server.keyboards);
  wl_list_init(&server.layers);

  server.current_workspace = 1;

  server.display = wl_display_create();

  server.backend =
      wlr_backend_autocreate(wl_display_get_event_loop(server.display), NULL);
  if (!server.backend) {
    fprintf(stderr, "Failed to create backend\n");
    return 1;
  }

  server.renderer = wlr_renderer_autocreate(server.backend);
  wlr_renderer_init_wl_display(server.renderer, server.display);

  server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);

  server.compositor = wlr_compositor_create(server.display, 5, server.renderer);
  wlr_subcompositor_create(server.display);
  wlr_data_device_manager_create(server.display);

  server.output_layout = wlr_output_layout_create(server.display);

  // Scene setup
  server.scene = wlr_scene_create();
  server.layer_bg = wlr_scene_tree_create(&server.scene->tree);
  server.layer_bottom = wlr_scene_tree_create(&server.scene->tree);
  server.layer_windows = wlr_scene_tree_create(&server.scene->tree);
  server.layer_top = wlr_scene_tree_create(&server.scene->tree);
  server.layer_overlay = wlr_scene_tree_create(&server.scene->tree);
  server.scene_layout =
      wlr_scene_attach_output_layout(server.scene, server.output_layout);

  // Layer shell
  server.layer_shell = wlr_layer_shell_v1_create(server.display, 4);
  server.new_layer_surface.notify = server_new_layer_surface;
  wl_signal_add(&server.layer_shell->events.new_surface,
                &server.new_layer_surface);

  // XDG output
  wlr_xdg_output_manager_v1_create(server.display, server.output_layout);

  // Screencopy (already exists - keep it)
  wlr_screencopy_manager_v1_create(server.display);

  // *** NEW: Clipboard support ***
  if (clipboard_init(&server) < 0) {
    fprintf(stderr, "Warning: Clipboard initialization failed\n");
  }

  // *** NEW: Idle and locking support ***
  if (idle_init(&server) < 0) {
    fprintf(stderr, "Warning: Idle management initialization failed\n");
  }
  if (session_lock_init(&server) < 0) {
    fprintf(stderr, "Warning: Session lock initialization failed\n");
  }

  // *** NEW: Multi-monitor support ***
  if (multimonitor_init(&server) < 0) {
    fprintf(stderr, "Warning: Multi-monitor initialization failed\n");
  }

  // *** NEW: IME support ***
  if (ime_init(&server) < 0) {
    fprintf(stderr, "Warning: IME initialization failed\n");
  }
  if (foreign_toplevel_init(&server) < 0) {
    fprintf(stderr, "Warning: Foreign toplevel initialization failed\n");
  }

  if (pointer_constraints_init(&server) < 0) {
    fprintf(stderr, "Warning: Pointer constraints initialization failed\n");
  }

  if (keyboard_shortcuts_inhibit_init(&server) < 0) {
    fprintf(stderr,
            "Warning: Keyboard shortcuts inhibit initialization failed\n");
  }

  if (fractional_scale_init(&server) < 0) {
    fprintf(stderr, "Warning: Fractional scale initialization failed\n");
  }

  if (xdg_activation_init(&server) < 0) {
    fprintf(stderr, "Warning: XDG activation initialization failed\n");
  }

  if (session_lock_init_v2(&server) < 0) {
    fprintf(stderr, "Warning: Session lock v2 initialization failed\n");
  }

  if (presentation_time_init(&server) < 0) {
    fprintf(stderr, "Warning: Presentation time initialization failed\n");
  }

  if (viewporter_init(&server) < 0) {
    fprintf(stderr, "Warning: Viewporter initialization failed\n");
  }
  // XDG decoration (already exists - keep it)
  server.xdg_decoration_mgr =
      wlr_xdg_decoration_manager_v1_create(server.display);
  server.new_xdg_decoration.notify = server_new_xdg_decoration;
  wl_signal_add(&server.xdg_decoration_mgr->events.new_toplevel_decoration,
                &server.new_xdg_decoration);

  // Output handlers (already exists - keep it)
  server.new_output.notify = server_new_output;
  wl_signal_add(&server.backend->events.new_output, &server.new_output);

  // XDG shell (already exists - keep it)
  server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
  server.new_xdg_toplevel.notify = server_new_xdg_toplevel;
  wl_signal_add(&server.xdg_shell->events.new_toplevel,
                &server.new_xdg_toplevel);
  server.new_xdg_popup.notify = server_new_xdg_popup;
  wl_signal_add(&server.xdg_shell->events.new_popup, &server.new_xdg_popup);

  // Cursor and input (already exists - keep it)
  server.cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
  server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  server.cursor_motion.notify = cursor_motion;
  wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
  server.cursor_motion_abs.notify = cursor_motion_abs;
  wl_signal_add(&server.cursor->events.motion_absolute,
                &server.cursor_motion_abs);
  server.cursor_button.notify = cursor_button;
  wl_signal_add(&server.cursor->events.button, &server.cursor_button);
  server.cursor_axis.notify = cursor_axis;
  wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
  server.cursor_frame.notify = cursor_frame;
  wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

  // Gestures (already exists - keep it)
  server.gesture_swipe_begin.notify = gesture_swipe_begin;
  wl_signal_add(&server.cursor->events.swipe_begin,
                &server.gesture_swipe_begin);
  server.gesture_swipe_update.notify = gesture_swipe_update;
  wl_signal_add(&server.cursor->events.swipe_update,
                &server.gesture_swipe_update);
  server.gesture_swipe_end.notify = gesture_swipe_end;
  wl_signal_add(&server.cursor->events.swipe_end, &server.gesture_swipe_end);

  server.gesture_pinch_begin.notify = gesture_pinch_begin;
  wl_signal_add(&server.cursor->events.pinch_begin,
                &server.gesture_pinch_begin);
  server.gesture_pinch_update.notify = gesture_pinch_update;
  wl_signal_add(&server.cursor->events.pinch_update,
                &server.gesture_pinch_update);
  server.gesture_pinch_end.notify = gesture_pinch_end;
  wl_signal_add(&server.cursor->events.pinch_end, &server.gesture_pinch_end);

  server.gesture_hold_begin.notify = gesture_hold_begin;
  wl_signal_add(&server.cursor->events.hold_begin, &server.gesture_hold_begin);
  server.gesture_hold_end.notify = gesture_hold_end;
  wl_signal_add(&server.cursor->events.hold_end, &server.gesture_hold_end);

  server.new_input.notify = new_input;
  wl_signal_add(&server.backend->events.new_input, &server.new_input);

  server.seat = wlr_seat_create(server.display, "seat0");
  server.request_cursor.notify = request_cursor;
  wl_signal_add(&server.seat->events.request_set_cursor,
                &server.request_cursor);

  // Load config (already exists - keep it)
  char config_file[512];
  snprintf(config_file, sizeof(config_file),
           "%s/.config/starview/starview.toml", getenv("HOME"));
  config_load(config_file);

  // Titlebar theme (already exists - keep it)
  struct titlebar_theme *theme = titlebar_theme_create();
  titlebar_theme_load_from_config(theme, &config.decor);
  titlebar_set_global_theme(theme);
  printf("✓ Titlebar theme initialized from config\n");

  // IPC (already exists - keep it)
  if (ipc_init(&server) < 0) {
    fprintf(stderr, "Warning: Failed to initialize IPC\n");
  }

  // Set mode from config (already exists - keep it)
  server.mode = config.default_mode;
  server.preselect = PRESELECT_NONE;

  // Run autostart (already exists - keep it)
  for (int i = 0; i < config.autostart_count; i++) {
    if (fork() == 0) {
      execl("/bin/sh", "sh", "-c", config.autostart[i], NULL);
      _exit(1);
    }
  }

  const char *socket = wl_display_add_socket_auto(server.display);
  if (!socket) {
    fprintf(stderr, "Failed to create socket\n");
    return 1;
  }

  if (!wlr_backend_start(server.backend)) {
    fprintf(stderr, "Failed to start backend\n");
    return 1;
  }

  // *** NEW: Initialize XWayland (do this AFTER backend starts) ***
  if (xwayland_init(&server) < 0) {
    fprintf(stderr, "Warning: XWayland initialization failed\n");
  }

  setenv("WAYLAND_DISPLAY", socket, 1);
  printf("Running on WAYLAND_DISPLAY=%s\n", socket);
  printf("Mode: %s\n", server.mode == MODE_TILING ? "TILING" : "FLOATING");
  printf("Gesture support enabled\n");

  // Update backgrounds (already exists - keep it)
  struct output *output;
  wl_list_for_each(output, &server.outputs, link) {
    if (output->background && config.background.enabled) {
      int width = output->wlr_output->width;
      int height = output->wlr_output->height;
      background_update(output->background, width, height, &config.background);
    }
  }

  // Launch terminal (already exists - keep it)
  pid_t pid = fork();
  if (pid == 0) {
    execlp("foot", "foot", NULL);
    execlp("kitty", "kitty", NULL);
    _exit(1);
  }

  wl_display_run(server.display);

  // Cleanup
  ipc_finish(&server);
  xwayland_finish(&server);
  wl_display_destroy(server.display);
  return 0;
}
