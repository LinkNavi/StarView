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
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
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

/*
 * ANIMATION STATE
 */
struct animation {
    bool active;
    enum anim_type type;
    int64_t start_time;
    int duration_ms;
    
    // Start/end values
    float start_x, start_y;
    float end_x, end_y;
    float start_w, start_h;
    float end_w, end_h;
    float start_opacity;
    float end_opacity;
    float start_scale;
    float end_scale;
    
    // Callback when done
    void (*on_complete)(void *data);
    void *data;
};

/*
 * DECORATION BUTTON HIT
 */
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

struct layer_surface {
    struct server *server;
    struct wlr_layer_surface_v1 *wlr_layer;
    struct wlr_scene_tree *scene_tree;
    struct wl_listener map;
    struct wl_listener unmap;
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
    
    // Scene layers (bottom to top)
    struct wlr_scene_tree *layer_bg;
    struct wlr_scene_tree *layer_bottom;
    struct wlr_scene_tree *layer_windows;
    struct wlr_scene_tree *layer_top;
    struct wlr_scene_tree *layer_overlay;
    
    struct wl_list outputs;
    struct wl_list toplevels;
    struct wl_list keyboards;
    
    // Window management mode
    enum window_mode mode;
    int current_workspace;
    
    // Animation timer
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

/*
 * DECORATION STRUCTURE
 */
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
    
    // Decoration
    struct decoration decor;
    
    // Animation state
    struct animation anim;
    
    // Window state
    bool floating;
    bool fullscreen;
    bool minimized;
    bool maximized;
    int workspace;
    float opacity;
    
    // Saved geometry for mode switching / maximize
    int saved_x, saved_y;
    int saved_width, saved_height;
    
    // Pre-maximize geometry
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
                             struct wlr_surface **surface, double *sx, double *sy);

/* shell.c */
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
void cursor_button(struct wl_listener *listener, void *data);
bool handle_keybind(struct server *server, enum keybind_action action, const char *arg);

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
void anim_start(struct toplevel *toplevel, enum anim_type type,
                float end_x, float end_y, float end_w, float end_h,
                void (*on_complete)(void *), void *data);
void anim_start_opacity(struct toplevel *toplevel, float end_opacity,
                        void (*on_complete)(void *), void *data);
void anim_update(struct server *server);
bool anim_is_active(struct toplevel *toplevel);
float anim_ease(float t, enum anim_curve curve);
int64_t get_time_ms(void);

/* rules.c */
void apply_window_rules(struct toplevel *toplevel);

#endif
