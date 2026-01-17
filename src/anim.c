#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "core.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

float anim_ease(float t, enum anim_curve curve) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    switch (curve) {
    case CURVE_LINEAR:
        return t;
        
    case CURVE_EASE_IN:
        return t * t;
        
    case CURVE_EASE_OUT:
        return 1.0f - (1.0f - t) * (1.0f - t);
        
    case CURVE_EASE_IN_OUT:
        if (t < 0.5f) {
            return 2.0f * t * t;
        } else {
            return 1.0f - powf(-2.0f * t + 2.0f, 2) / 2.0f;
        }
        
    case CURVE_BOUNCE: {
        float n1 = 7.5625f;
        float d1 = 2.75f;
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }
    
    case CURVE_SPRING: {
        float c4 = (2.0f * M_PI) / 3.0f;
        return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f :
            powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
    }
    
    default:
        return t;
    }
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void anim_start(struct toplevel *toplevel, enum anim_type type,
                float end_x, float end_y, float end_w, float end_h,
                void (*on_complete)(void *), void *data) {
    if (!config.anim.enabled || type == ANIM_NONE) {
        // No animation, just set final values
        if (toplevel->decor.tree) {
            wlr_scene_node_set_position(&toplevel->decor.tree->node, (int)end_x, (int)end_y);
        } else {
            wlr_scene_node_set_position(&toplevel->scene_tree->node, (int)end_x, (int)end_y);
        }
        if (end_w > 0 && end_h > 0) {
            wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, (int)end_w, (int)end_h);
        }
        if (on_complete) on_complete(data);
        return;
    }
    
    struct animation *a = &toplevel->anim;
    
    // Get current position
    struct wlr_scene_node *node = toplevel->decor.tree ? 
        &toplevel->decor.tree->node : &toplevel->scene_tree->node;
    
    a->active = true;
    a->type = type;
    a->start_time = get_time_ms();
    a->duration_ms = config.anim.duration_ms;
    
    a->start_x = node->x;
    a->start_y = node->y;
    a->end_x = end_x;
    a->end_y = end_y;
    
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    a->start_w = geo.width > 0 ? geo.width : 100;
    a->start_h = geo.height > 0 ? geo.height : 100;
    a->end_w = end_w > 0 ? end_w : a->start_w;
    a->end_h = end_h > 0 ? end_h : a->start_h;
    
    a->start_opacity = toplevel->opacity;
    a->end_opacity = 1.0f;
    a->start_scale = 1.0f;
    a->end_scale = 1.0f;
    
    // Type-specific setup
    switch (type) {
    case ANIM_FADE:
        a->start_opacity = config.anim.fade_min;
        break;
    case ANIM_ZOOM:
        a->start_scale = config.anim.zoom_min;
        break;
    case ANIM_SLIDE_FADE:
        a->start_opacity = config.anim.fade_min;
        break;
    case ANIM_SLIDE_UP:
        a->start_y = end_y + 50;
        break;
    case ANIM_SLIDE_DOWN:
        a->start_y = end_y - 50;
        break;
    case ANIM_SLIDE_LEFT:
        a->start_x = end_x + 50;
        break;
    case ANIM_SLIDE_RIGHT:
        a->start_x = end_x - 50;
        break;
    default:
        break;
    }
    
    a->on_complete = on_complete;
    a->data = data;
}

void anim_start_opacity(struct toplevel *toplevel, float end_opacity,
                        void (*on_complete)(void *), void *data) {
    if (!config.anim.enabled) {
        toplevel->opacity = end_opacity;
        // TODO: Apply opacity to scene node when wlroots supports it
        if (on_complete) on_complete(data);
        return;
    }
    
    struct animation *a = &toplevel->anim;
    
    a->active = true;
    a->type = ANIM_FADE;
    a->start_time = get_time_ms();
    a->duration_ms = config.anim.duration_ms;
    
    a->start_opacity = toplevel->opacity;
    a->end_opacity = end_opacity;
    
    a->on_complete = on_complete;
    a->data = data;
}

bool anim_is_active(struct toplevel *toplevel) {
    return toplevel->anim.active;
}

void anim_update(struct server *server) {
    int64_t now = get_time_ms();
    bool need_redraw = false;
    
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        struct animation *a = &toplevel->anim;
        if (!a->active) continue;
        
        need_redraw = true;
        
        float elapsed = (float)(now - a->start_time);
        float t = elapsed / (float)a->duration_ms;
        
        if (t >= 1.0f) {
            // Animation complete
            t = 1.0f;
            a->active = false;
        }
        
        float eased = anim_ease(t, config.anim.curve);
        
        // Apply animation based on type
        struct wlr_scene_node *node = toplevel->decor.tree ? 
            &toplevel->decor.tree->node : &toplevel->scene_tree->node;
        
        switch (a->type) {
        case ANIM_SLIDE:
        case ANIM_SLIDE_FADE:
        case ANIM_SLIDE_UP:
        case ANIM_SLIDE_DOWN:
        case ANIM_SLIDE_LEFT:
        case ANIM_SLIDE_RIGHT: {
            int x = (int)lerp(a->start_x, a->end_x, eased);
            int y = (int)lerp(a->start_y, a->end_y, eased);
            wlr_scene_node_set_position(node, x, y);
            
            if (a->type == ANIM_SLIDE_FADE) {
                toplevel->opacity = lerp(a->start_opacity, a->end_opacity, eased);
            }
            break;
        }
        
        case ANIM_FADE:
            toplevel->opacity = lerp(a->start_opacity, a->end_opacity, eased);
            break;
            
        case ANIM_ZOOM: {
            float scale = lerp(a->start_scale, a->end_scale, eased);
            int w = (int)(a->end_w * scale);
            int h = (int)(a->end_h * scale);
            
            // Center the scaled window
            int x = (int)(a->end_x + (a->end_w - w) / 2);
            int y = (int)(a->end_y + (a->end_h - h) / 2);
            
            wlr_scene_node_set_position(node, x, y);
            if (w > 50 && h > 50) {
                wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, w, h);
            }
            break;
        }
        
        default:
            break;
        }
        
        // Animation complete callback
        if (!a->active && a->on_complete) {
            // Set final values
            wlr_scene_node_set_position(node, (int)a->end_x, (int)a->end_y);
            if (a->end_w > 0 && a->end_h > 0) {
                wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 
                    (int)a->end_w, (int)a->end_h);
            }
            toplevel->opacity = a->end_opacity;
            
            a->on_complete(a->data);
            a->on_complete = NULL;
        }
    }
    
    (void)need_redraw;
}

void anim_schedule_update(struct server *server) {
    if (!server->anim_timer) {
        struct wl_event_loop *loop = wl_display_get_event_loop(server->display);
        server->anim_timer = wl_event_loop_add_timer(loop, 
            (int (*)(void *))anim_update, server);
    }
    wl_event_source_timer_update(server->anim_timer, 16);
}
