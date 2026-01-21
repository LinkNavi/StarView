#ifndef CORE_H
#define CORE_H

#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "config.h"

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
  double grab_x, grab_y;
  struct wlr_box grab_box;
  enum cursor_mode mode;
  uint32_t resize_edges;
};

extern struct cursor_state cursor_state;

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

  struct wlr_layer_shell_v1 *layer_shell;
  struct wl_listener new_layer_surface;

  struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
  struct wl_listener new_xdg_decoration;

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
  struct wl_list layers;  /* list of layer_surface */

  enum window_mode mode;
  int current_workspace;

  struct wl_event_source *anim_timer;
};

struct output {
  struct wl_list link;
  struct server *server;
  struct wlr_output *wlr_output;
  struct wlr_scene_output *scene_output;
  struct wl_listener frame;
  struct wl_listener request_state;
  struct wl_listener destroy;
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

  /* Cairo-rendered titlebar */
  struct rendered_titlebar *rendered_titlebar;

  int width;
  bool hovered_close;
  bool hovered_max;
  bool hovered_min;
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

#endif
