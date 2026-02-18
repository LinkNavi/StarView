#ifndef CORE_H
#define CORE_H

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"
#include "window.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

enum cursor_mode {
  CURSOR_NORMAL,
  CURSOR_MOVE,
  CURSOR_RESIZE,
  CURSOR_RESIZE_TOP,
  CURSOR_RESIZE_BOTTOM,
  CURSOR_RESIZE_LEFT,
  CURSOR_RESIZE_RIGHT,
  CURSOR_RESIZE_TOP_LEFT,
  CURSOR_RESIZE_TOP_RIGHT,
  CURSOR_RESIZE_BOTTOM_LEFT,
  CURSOR_RESIZE_BOTTOM_RIGHT,
};

struct animation {
  bool active;
  enum anim_type type;
  int64_t start_time;
  int duration_ms;

  float start_x, start_y;
  float end_x, end_y;
  float start_w, start_h;
  float end_w, end_h;
  float start_opacity;
  float end_opacity;
  float start_scale;
  float end_scale;

  void (*on_complete)(void *data);
  void *data;
};

enum decor_hit {
  HIT_NONE = 0,
  HIT_TITLEBAR,
  HIT_CLOSE,
  HIT_MAXIMIZE,
  HIT_MINIMIZE,
  HIT_RESIZE_TOP,
  HIT_RESIZE_BOTTOM,
  HIT_RESIZE_LEFT,
  HIT_RESIZE_RIGHT,
  HIT_RESIZE_TOP_LEFT,
  HIT_RESIZE_TOP_RIGHT,
  HIT_RESIZE_BOTTOM_LEFT,
  HIT_RESIZE_BOTTOM_RIGHT,
};

enum titlebar_element_type {
  TITLEBAR_BUTTON,
  TITLEBAR_LABEL,
  TITLEBAR_INDICATOR,
  TITLEBAR_SPACER,
  TITLEBAR_CUSTOM,
};

struct toplevel;

struct titlebar_element {
  enum titlebar_element_type type;
  struct wlr_scene_rect *rect;
  int x, y;
  int width, height;
  uint32_t color;
  char text[128];
  bool visible;
  bool enabled;
  void (*on_click)(struct toplevel *);
};

struct titlebar {
  struct wlr_scene_tree *tree;
  struct wlr_scene_rect *background;
  int width;

  struct titlebar_element *btn_close;
  struct titlebar_element *btn_max;
  struct titlebar_element *btn_min;

  struct titlebar_element *elements[32];
  int element_count;
};

struct layer_surface {
  struct wl_list link;
  struct server *server;
  struct wlr_layer_surface_v1 *wlr_layer;
  struct wlr_scene_tree *scene_tree;
  struct wlr_scene_layer_surface_v1 *scene_layer;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener commit;
  struct wl_listener destroy;
};

struct cursor_state {
  struct toplevel *toplevel;
  struct xwayland_surface *xwayland;
  double grab_x, grab_y;
  struct wlr_box grab_box;
  enum cursor_mode mode;
  uint32_t resize_edges;
};

extern struct cursor_state cursor_state;

enum preselect_dir {
  PRESELECT_NONE,
  PRESELECT_LEFT,
  PRESELECT_RIGHT,
  PRESELECT_UP,
  PRESELECT_DOWN,
};

/* Forward declarations for protocol types */
struct wlr_foreign_toplevel_manager_v1;
struct wlr_pointer_constraints_v1;
struct wlr_relative_pointer_manager_v1;
struct wlr_keyboard_shortcuts_inhibit_manager_v1;
struct wlr_fractional_scale_manager_v1;
struct wlr_xdg_activation_v1;
struct wlr_session_lock_manager_v1;
struct wlr_session_lock_v1;
struct wlr_presentation;
struct wlr_viewporter;

struct server {
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;
  struct wlr_scene *scene;
  struct wlr_scene_output_layout *scene_layout;
  struct wlr_output_layout *output_layout;
  struct wlr_xdg_shell *xdg_shell;
  struct wlr_seat *seat;
  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
  enum preselect_dir preselect;

  /* Gestures */
  struct wl_listener gesture_swipe_begin;
  struct wl_listener gesture_swipe_update;
  struct wl_listener gesture_swipe_end;
  struct wl_listener gesture_pinch_begin;
  struct wl_listener gesture_pinch_update;
  struct wl_listener gesture_pinch_end;
  struct wl_listener gesture_hold_begin;
  struct wl_listener gesture_hold_end;

  /* XWayland */
  struct wlr_xwayland *xwayland;
  struct wl_listener new_xwayland_surface;
  struct wl_listener xwayland_ready;
  struct wl_list xwayland_surfaces;

  /* Layer shell */
  struct wlr_layer_shell_v1 *layer_shell;
  struct wl_listener new_layer_surface;

  /* Compositor */
  struct wlr_compositor *compositor;
  struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
  struct wl_listener new_xdg_decoration;
  
  /* Clipboard */
  struct wlr_primary_selection_v1_device_manager *primary_selection;
  struct wlr_data_control_manager_v1 *data_control;
  
  /* Idle and locking (legacy) */
  struct wlr_idle_notifier_v1 *idle_notifier;
  struct wlr_idle_inhibit_v1 *idle_inhibit;
  struct wlr_session_lock_manager_v1 *session_lock_mgr;
  struct wl_listener new_session_lock;
  struct wlr_session_lock_v1 *locked;
  
  /* Multi-monitor */
  struct wlr_output_manager_v1 *output_manager;
  struct wlr_output_power_manager_v1 *output_power_manager;
  struct wl_listener output_manager_apply;
  struct wl_listener output_manager_test;
  struct wl_listener output_power_mgr_set_mode;
  
  /* IME */
  struct wlr_text_input_manager_v3 *text_input;
  struct wlr_input_method_manager_v2 *input_method_mgr;
  struct wl_listener new_text_input;
  struct wl_listener new_input_method;
  struct wlr_input_method_v2 *input_method;
  struct wlr_text_input_v3 *active_text_input;

  /* === NEW PROTOCOLS === */

  /* Foreign toplevel management (waybar) */
  struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel_mgr;

  /* Pointer constraints + relative pointer (games) */
  struct wlr_pointer_constraints_v1 *pointer_constraints;
  struct wlr_relative_pointer_manager_v1 *relative_pointer_mgr;
  struct wl_listener new_pointer_constraint;

  /* Keyboard shortcuts inhibit (games/VMs) */
  struct wlr_keyboard_shortcuts_inhibit_manager_v1 *keyboard_shortcuts_inhibit;
  struct wl_listener new_keyboard_shortcuts_inhibit;

  /* Fractional scale (HiDPI) */
  struct wlr_fractional_scale_manager_v1 *fractional_scale_mgr;

  /* XDG activation (notification focus) */
  struct wlr_xdg_activation_v1 *xdg_activation;
  struct wl_listener xdg_activation_request;

  /* Session lock v2 (ext-session-lock, full impl) */
  struct wlr_session_lock_manager_v1 *session_lock_mgr_v2;
  struct wl_listener new_session_lock_v2;
  struct wlr_session_lock_v1 *locked_session;
  struct wl_listener session_lock_new_surface;
  struct wl_listener session_lock_unlock;
  struct wl_listener session_lock_destroy;

  /* Presentation time (video) */
  struct wlr_presentation *presentation;

  /* Viewporter (video) */
  struct wlr_viewporter *viewporter;

  /* === END NEW PROTOCOLS === */

  struct wl_listener new_output;
  struct wl_listener new_xdg_toplevel;
  struct wl_listener new_xdg_popup;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_abs;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;
  struct wl_listener new_input;
  struct wl_listener request_cursor;

  struct wlr_scene_tree *layer_bg;
  struct wlr_scene_tree *layer_bottom;
  struct wlr_scene_tree *layer_windows;
  struct wlr_scene_tree *layer_top;
  struct wlr_scene_tree *layer_overlay;

  struct wl_list outputs;
  struct wl_list toplevels;
  struct wl_list keyboards;
  struct wl_list layers;

  /* Unified window list (NEW - use alongside toplevels during migration) */
  struct wl_list windows;

  enum window_mode mode;
  int current_workspace;

  struct wl_event_source *anim_timer;
};

void decor_set_position(struct toplevel *toplevel, int x, int y);

struct output {
  struct wl_list link;
  struct server *server;
  int current_workspace;
  char name[64];
  char description[128];
  struct wlr_output *wlr_output;
  struct wlr_scene_output *scene_output;
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;

  struct wlr_scene_buffer *background;
};

struct rendered_titlebar;

struct decoration {
  struct wlr_scene_tree *tree;
  struct wlr_scene_rect *titlebar;
  struct wlr_scene_rect *btn_close;
  struct wlr_scene_rect *btn_max;
  struct wlr_scene_rect *btn_min;
  struct wlr_scene_rect *border_top;
  struct wlr_scene_rect *border_bottom;
  struct wlr_scene_rect *border_left;
  struct wlr_scene_rect *border_right;
  struct wlr_scene_buffer *shadow;

  struct rendered_titlebar *rendered_titlebar;

  int width;
  bool hovered_close;
  bool hovered_max;
  bool hovered_min;

  int cached_shadow_width;
  int cached_shadow_height;
};

struct toplevel {
  struct wl_list link;
  struct server *server;
  struct wlr_xdg_toplevel *xdg_toplevel;
  struct wlr_scene_tree *scene_tree;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener commit;
  struct wl_listener destroy;
  struct wl_listener request_fullscreen;
  struct wl_listener request_minimize;

  struct decoration decor;
  struct animation anim;

  bool floating;
  bool fullscreen;
  bool minimized;
  bool maximized;
  int workspace;
  float opacity;

  int saved_x, saved_y;
  int saved_width, saved_height;

  int pre_max_x, pre_max_y;
  int pre_max_width, pre_max_height;
};

struct keyboard {
  struct wl_list link;
  struct server *server;
  struct wlr_keyboard *wlr_keyboard;
  struct wl_listener key;
  struct wl_listener destroy;
  struct wl_listener modifiers;
};

/* XWayland surface - kept for backward compat during migration */
struct xwayland_surface {
  struct wl_list link;
  struct server *server;
  struct wlr_xwayland_surface *xwayland_surface;
  struct wlr_scene_tree *scene_tree;
  
  bool override_redirect;
  bool is_popup;
  bool is_dialog;
  bool is_splash;
  struct xwayland_surface *parent;
  
  bool floating;
  bool fullscreen;
  bool minimized;
  bool maximized;
  int workspace;
  struct decoration decor;
  struct animation anim;
  float opacity;
  int saved_x, saved_y, saved_width, saved_height;
  int pre_max_x, pre_max_y, pre_max_width, pre_max_height;
  
  /* Flag: set true during destroy to prevent double-free */
  bool destroying;
  
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener surface_map;
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
};

/* core.c */
void server_new_output(struct wl_listener *listener, void *data);
void cursor_motion(struct wl_listener *listener, void *data);
void cursor_motion_abs(struct wl_listener *listener, void *data);
void cursor_axis(struct wl_listener *listener, void *data);
void cursor_frame(struct wl_listener *listener, void *data);
void request_cursor(struct wl_listener *listener, void *data);
void new_input(struct wl_listener *listener, void *data);
void server_new_xdg_popup(struct wl_listener *listener, void *data);
void server_new_xdg_decoration(struct wl_listener *listener, void *data);
void server_new_layer_surface(struct wl_listener *listener, void *data);

/* Protocol init (legacy) */
int clipboard_init(struct server *server);
int idle_init(struct server *server);
int session_lock_init(struct server *server);
int multimonitor_init(struct server *server);
int ime_init(struct server *server);
int xwayland_init(struct server *server);
void xwayland_finish(struct server *server);

/* XWayland helpers */
struct xwayland_surface *xwayland_surface_at(struct server *server,
                                              double lx, double ly,
                                              struct wlr_surface **surface,
                                              double *sx, double *sy);
void xwayland_surface_focus(struct xwayland_surface *xsurface);
void xwayland_surface_raise(struct xwayland_surface *xsurface);
void xwayland_surface_lower(struct xwayland_surface *xsurface);

struct toplevel *toplevel_at(struct server *server, double lx, double ly,
                             struct wlr_surface **surface, double *sx,
                             double *sy);

/* shell.c */
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
void cursor_button(struct wl_listener *listener, void *data);
bool handle_keybind(struct server *server, enum keybind_action action,
                    const char *arg);

void arrange_windows(struct server *server);
void focus_toplevel(struct toplevel *toplevel);
struct toplevel *get_focused_toplevel(struct server *server);

/* decor.c */
void decor_create(struct toplevel *toplevel);
void decor_update(struct toplevel *toplevel, bool focused);
void decor_destroy(struct toplevel *toplevel);
void decor_set_size(struct toplevel *toplevel, int width);
enum decor_hit decor_hit_test(struct toplevel *toplevel, double x, double y);
void decor_update_hover(struct toplevel *toplevel, double x, double y);

/* anim.c */
void anim_start(struct toplevel *toplevel, enum anim_type type, float end_x,
                float end_y, float end_w, float end_h,
                void (*on_complete)(void *), void *data);
void anim_start_opacity(struct toplevel *toplevel, float end_opacity,
                        void (*on_complete)(void *), void *data);
int anim_update(void *data);
bool anim_is_active(struct toplevel *toplevel);
float anim_ease(float t, enum anim_curve curve);
int64_t get_time_ms(void);
void anim_schedule_update(struct server *server);

/* rules.c */
void apply_window_rules(struct toplevel *toplevel);

/* titlebar.c */
struct titlebar_element *
titlebar_element_create(struct titlebar *titlebar,
                        enum titlebar_element_type type, int x, int y,
                        int width, int height, uint32_t color);

struct titlebar_element *
titlebar_button_create(struct titlebar *titlebar, int x, int y, uint32_t color,
                       void (*on_click)(struct toplevel *));

struct titlebar_element *titlebar_label_create(struct titlebar *titlebar, int x,
                                               int y, int width,
                                               const char *text,
                                               uint32_t color);

struct titlebar_element *titlebar_indicator_create(struct titlebar *titlebar,
                                                   int x, int y, int size,
                                                   uint32_t color);

struct titlebar_element *titlebar_spacer_create(struct titlebar *titlebar,
                                                int x, int y, int width);

void titlebar_element_set_position(struct titlebar_element *elem, int x, int y);
void titlebar_element_set_size(struct titlebar_element *elem, int width,
                               int height);
void titlebar_element_set_color(struct titlebar_element *elem, uint32_t color);
void titlebar_element_set_visible(struct titlebar_element *elem, bool visible);
void titlebar_element_set_text(struct titlebar_element *elem, const char *text);
bool titlebar_element_contains(struct titlebar_element *elem, int x, int y);

void titlebar_layout_horizontal(struct titlebar *titlebar,
                                struct titlebar_element **elements, int count,
                                int start_x, int spacing);
void titlebar_layout_vertical(struct titlebar *titlebar,
                              struct titlebar_element **elements, int count,
                              int start_y, int spacing);
void titlebar_layout_right_align(struct titlebar *titlebar,
                                 struct titlebar_element **elements, int count,
                                 int spacing);
void titlebar_layout_center(struct titlebar *titlebar,
                            struct titlebar_element **elements, int count,
                            int spacing);

struct titlebar_element *titlebar_element_at(struct titlebar *titlebar, int x,
                                             int y);
void titlebar_element_click(struct titlebar_element *elem,
                            struct toplevel *toplevel);

void titlebar_apply_preset_windows(struct titlebar *titlebar);
void titlebar_apply_preset_macos(struct titlebar *titlebar);
void titlebar_apply_preset_minimal(struct titlebar *titlebar);
void titlebar_elements_destroy(struct titlebar *titlebar);

/* util.c */
struct wlr_box toplevel_get_geometry(struct toplevel *toplevel);
void toplevel_set_geometry(struct toplevel *toplevel, struct wlr_box geo);
bool toplevel_contains_point(struct toplevel *toplevel, int x, int y);
struct wlr_box box_intersection(struct wlr_box a, struct wlr_box b);
bool box_overlaps(struct wlr_box a, struct wlr_box b);

struct output *output_at(struct server *server, int x, int y);
struct output *output_get_primary(struct server *server);
struct wlr_box output_get_usable_area(struct output *output);

int server_count_toplevels(struct server *server, int workspace);
int server_count_visible_toplevels(struct server *server);
struct toplevel *server_find_toplevel_by_app_id(struct server *server,
                                                const char *app_id);
struct toplevel *server_find_toplevel_by_title(struct server *server,
                                               const char *title);

void workspace_show(struct server *server, int workspace);
void workspace_move_toplevel(struct toplevel *toplevel, int workspace);
bool workspace_is_empty(struct server *server, int workspace);

struct toplevel *focus_get_next(struct server *server,
                                struct toplevel *current);
struct toplevel *focus_get_prev(struct server *server,
                                struct toplevel *current);
struct toplevel *focus_find_urgent(struct server *server);

void toplevel_animate_move(struct toplevel *toplevel, int x, int y);
void toplevel_animate_resize(struct toplevel *toplevel, int width, int height);
void toplevel_animate_fade_in(struct toplevel *toplevel);
void toplevel_animate_fade_out(struct toplevel *toplevel,
                               void (*on_complete)(void *), void *data);

uint32_t color_lerp(uint32_t a, uint32_t b, float t);
uint32_t color_brighten(uint32_t color, float amount);

char *string_copy(const char *str);
bool string_starts_with(const char *str, const char *prefix);
bool string_ends_with(const char *str, const char *suffix);

int clamp_int(int value, int min, int max);
float clamp_float(float value, float min, float max);
float lerp_float(float a, float b, float t);
int lerp_int(int a, int b, float t);

void *timer_schedule(struct server *server, int ms, void (*callback)(void *),
                     void *data);
void timer_cancel(void *timer_handle);
void xwayland_surface_configure(struct xwayland_surface *xsurface,
                                 int x, int y, int width, int height);
#endif
