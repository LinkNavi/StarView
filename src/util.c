#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ipc.h"
// Window geometry helpers
struct wlr_box toplevel_get_geometry(struct toplevel *toplevel) {
    struct wlr_box geo = {0};
    if (!toplevel) return geo;
    
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    
    struct wlr_scene_node *node = toplevel->decor.tree ?
        &toplevel->decor.tree->node : &toplevel->scene_tree->node;
    geo.x = node->x;
    geo.y = node->y;
    
    return geo;
}

void toplevel_set_geometry(struct toplevel *toplevel, struct wlr_box geo) {
    if (!toplevel) return;
    
    struct wlr_scene_node *node = toplevel->decor.tree ?
        &toplevel->decor.tree->node : &toplevel->scene_tree->node;
    wlr_scene_node_set_position(node, geo.x, geo.y);
    
    if (geo.width > 0 && geo.height > 0) {
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, geo.width, geo.height);
    }
}

bool toplevel_contains_point(struct toplevel *toplevel, int x, int y) {
    if (!toplevel) return false;
    
    struct wlr_box geo = toplevel_get_geometry(toplevel);
    return wlr_box_contains_point(&geo, x, y);
}

struct wlr_box box_intersection(struct wlr_box a, struct wlr_box b) {
    struct wlr_box result;
    result.x = fmax(a.x, b.x);
    result.y = fmax(a.y, b.y);
    result.width = fmin(a.x + a.width, b.x + b.width) - result.x;
    result.height = fmin(a.y + a.height, b.y + b.height) - result.y;
    
    if (result.width < 0) result.width = 0;
    if (result.height < 0) result.height = 0;
    
    return result;
}

bool box_overlaps(struct wlr_box a, struct wlr_box b) {
    struct wlr_box inter = box_intersection(a, b);
    return inter.width > 0 && inter.height > 0;
}

// Output helpers
struct output *output_at(struct server *server, int x, int y) {
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        struct wlr_box box = {
            .x = 0, .y = 0,
            .width = output->wlr_output->width,
            .height = output->wlr_output->height
        };
        if (wlr_box_contains_point(&box, x, y)) {
            return output;
        }
    }
    return NULL;
}

struct output *output_get_primary(struct server *server) {
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        return output;
    }
    return NULL;
}

struct wlr_box output_get_usable_area(struct output *output) {
    struct wlr_box box = {
        .x = config.gaps_outer,
        .y = config.gaps_outer,
        .width = output->wlr_output->width - config.gaps_outer * 2,
        .height = output->wlr_output->height - config.gaps_outer * 2
    };
    return box;
}

// Toplevel collection helpers
int server_count_toplevels(struct server *server, int workspace) {
    int count = 0;
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (workspace < 0 || toplevel->workspace == workspace) {
            count++;
        }
    }
    return count;
}

int server_count_visible_toplevels(struct server *server) {
    int count = 0;
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (toplevel->workspace == server->current_workspace && 
            !toplevel->minimized && !toplevel->fullscreen) {
            count++;
        }
    }
    return count;
}

struct toplevel *server_find_toplevel_by_app_id(struct server *server, const char *app_id) {
    if (!app_id) return NULL;
    
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        const char *tid = toplevel->xdg_toplevel->app_id;
        if (tid && strcmp(tid, app_id) == 0) {
            return toplevel;
        }
    }
    return NULL;
}

struct toplevel *server_find_toplevel_by_title(struct server *server, const char *title) {
    if (!title) return NULL;
    
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        const char *ttitle = toplevel->xdg_toplevel->title;
        if (ttitle && strstr(ttitle, title)) {
            return toplevel;
        }
    }
    return NULL;
}

// Workspace helpers
void workspace_show(struct server *server, int workspace) {
    if (workspace < 1 || workspace > MAX_WORKSPACES) return;
    
    server->current_workspace = workspace;
    
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        bool visible = (toplevel->workspace == workspace);
        struct wlr_scene_node *node = toplevel->decor.tree ?
            &toplevel->decor.tree->node : &toplevel->scene_tree->node;
        wlr_scene_node_set_enabled(node, visible);
    }
     ipc_event_workspace(server);
    arrange_windows(server);
}

void workspace_move_toplevel(struct toplevel *toplevel, int workspace) {
    if (!toplevel || workspace < 1 || workspace > MAX_WORKSPACES) return;
    
    toplevel->workspace = workspace;
    
    bool visible = (workspace == toplevel->server->current_workspace);
    struct wlr_scene_node *node = toplevel->decor.tree ?
        &toplevel->decor.tree->node : &toplevel->scene_tree->node;
    wlr_scene_node_set_enabled(node, visible);
}

bool workspace_is_empty(struct server *server, int workspace) {
    return server_count_toplevels(server, workspace) == 0;
}

// Focus helpers
struct toplevel *focus_get_next(struct server *server, struct toplevel *current) {
    if (wl_list_empty(&server->toplevels)) return NULL;
    
    if (!current) {
        return wl_container_of(server->toplevels.next, current, link);
    }
    
    struct wl_list *next = current->link.next;
    if (next == &server->toplevels) {
        next = server->toplevels.next;
    }
    
    return wl_container_of(next, current, link);
}

struct toplevel *focus_get_prev(struct server *server, struct toplevel *current) {
    if (wl_list_empty(&server->toplevels)) return NULL;
    
    if (!current) {
        return wl_container_of(server->toplevels.prev, current, link);
    }
    
    struct wl_list *prev = current->link.prev;
    if (prev == &server->toplevels) {
        prev = server->toplevels.prev;
    }
    
    return wl_container_of(prev, current, link);
}

struct toplevel *focus_find_urgent(struct server *server) {
    return NULL;
}

// Animation helpers
void toplevel_animate_move(struct toplevel *toplevel, int x, int y) {
    if (!toplevel) return;
    
    struct wlr_box geo = toplevel_get_geometry(toplevel);
    
    if (config.anim.enabled && config.anim.window_move != ANIM_NONE) {
        anim_start(toplevel, config.anim.window_move, x, y, geo.width, geo.height, NULL, NULL);
        anim_schedule_update(toplevel->server);
    } else {
        struct wlr_scene_node *node = toplevel->decor.tree ?
            &toplevel->decor.tree->node : &toplevel->scene_tree->node;
        wlr_scene_node_set_position(node, x, y);
    }
}

void toplevel_animate_resize(struct toplevel *toplevel, int width, int height) {
    if (!toplevel) return;
    
    struct wlr_box geo = toplevel_get_geometry(toplevel);
    
    if (config.anim.enabled && config.anim.window_resize != ANIM_NONE) {
        anim_start(toplevel, config.anim.window_resize, geo.x, geo.y, width, height, NULL, NULL);
        anim_schedule_update(toplevel->server);
    } else {
        wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, width, height);
    }
}

void toplevel_animate_fade_in(struct toplevel *toplevel) {
    if (!toplevel) return;
    
    if (config.anim.enabled && config.anim.window_open != ANIM_NONE) {
        anim_start_opacity(toplevel, 1.0f, NULL, NULL);
        anim_schedule_update(toplevel->server);
    } else {
        toplevel->opacity = 1.0f;
    }
}

void toplevel_animate_fade_out(struct toplevel *toplevel, void (*on_complete)(void *), void *data) {
    if (!toplevel) return;
    
    if (config.anim.enabled && config.anim.window_close != ANIM_NONE) {
        anim_start_opacity(toplevel, 0.0f, on_complete, data);
        anim_schedule_update(toplevel->server);
    } else {
        toplevel->opacity = 0.0f;
        if (on_complete) on_complete(data);
    }
}

// Color helpers
uint32_t color_lerp(uint32_t a, uint32_t b, float t) {
    uint8_t ar = (a >> 24) & 0xff;
    uint8_t ag = (a >> 16) & 0xff;
    uint8_t ab = (a >> 8) & 0xff;
    uint8_t aa = a & 0xff;
    
    uint8_t br = (b >> 24) & 0xff;
    uint8_t bg = (b >> 16) & 0xff;
    uint8_t bb = (b >> 8) & 0xff;
    uint8_t ba = b & 0xff;
    
    uint8_t r = ar + (br - ar) * t;
    uint8_t g = ag + (bg - ag) * t;
    uint8_t bl = ab + (bb - ab) * t;
    uint8_t al = aa + (ba - aa) * t;
    
    return (r << 24) | (g << 16) | (bl << 8) | al;
}

uint32_t color_brighten(uint32_t color, float amount) {
    float r = ((color >> 24) & 0xff) / 255.0f;
    float g = ((color >> 16) & 0xff) / 255.0f;
    float b = ((color >> 8) & 0xff) / 255.0f;
    float a = (color & 0xff) / 255.0f;
    
    r = fmin(1.0f, r + amount);
    g = fmin(1.0f, g + amount);
    b = fmin(1.0f, b + amount);
    
    return ((uint8_t)(r * 255) << 24) | 
           ((uint8_t)(g * 255) << 16) | 
           ((uint8_t)(b * 255) << 8) | 
           (uint8_t)(a * 255);
}



// String helpers
char *string_copy(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, str, len + 1);
    return copy;
}

bool string_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool string_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// Math helpers
int clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float lerp_float(float a, float b, float t) {
    return a + (b - a) * t;
}

int lerp_int(int a, int b, float t) {
    return (int)(a + (b - a) * t);
}

// Timer helpers
struct timer_data {
    struct wl_event_source *source;
    void (*callback)(void *);
    void *user_data;
};

static int timer_callback_wrapper(void *data) {
    struct timer_data *timer = data;
    if (timer->callback) {
        timer->callback(timer->user_data);
    }
    wl_event_source_remove(timer->source);
    free(timer);
    return 0;
}

void *timer_schedule(struct server *server, int ms, void (*callback)(void *), void *data) {
    struct timer_data *timer = calloc(1, sizeof(*timer));
    if (!timer) return NULL;
    
    timer->callback = callback;
    timer->user_data = data;
    
    struct wl_event_loop *loop = wl_display_get_event_loop(server->display);
    timer->source = wl_event_loop_add_timer(loop, timer_callback_wrapper, timer);
    if (!timer->source) {
        free(timer);
        return NULL;
    }
    
    wl_event_source_timer_update(timer->source, ms);
    return timer;
}

void timer_cancel(void *timer_handle) {
    if (!timer_handle) return;
    struct timer_data *timer = timer_handle;
    wl_event_source_remove(timer->source);
    free(timer);
}
