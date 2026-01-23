#ifndef GESTURE_H
#define GESTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Use the types already defined in config.h

enum gesture_type {
    GESTURE_NONE,
    GESTURE_SWIPE,
    GESTURE_PINCH,
    GESTURE_HOLD,
};

struct gesture_state {
    bool active;
    enum gesture_type type;
    uint32_t fingers;
    
    // Swipe
    double dx, dy;
    
    // Pinch
    double scale;
    double rotation;
    
    // Mouse gestures
    bool mouse_active;
    uint32_t mouse_button;
    uint32_t mouse_mods;
    double mouse_start_x, mouse_start_y;
    double mouse_dx, mouse_dy;
};

extern struct gesture_state gesture_state;

// Forward declaration
struct server;

// Touchpad gestures
void gesture_swipe_begin(struct wl_listener *listener, void *data);
void gesture_swipe_update(struct wl_listener *listener, void *data);
void gesture_swipe_end(struct wl_listener *listener, void *data);

void gesture_pinch_begin(struct wl_listener *listener, void *data);
void gesture_pinch_update(struct wl_listener *listener, void *data);
void gesture_pinch_end(struct wl_listener *listener, void *data);

void gesture_hold_begin(struct wl_listener *listener, void *data);
void gesture_hold_end(struct wl_listener *listener, void *data);

// Mouse gestures
void mouse_gesture_begin(struct server *server, uint32_t button, uint32_t mods);
void mouse_gesture_update(struct server *server, double dx, double dy);
bool mouse_gesture_end(struct server *server);
bool mouse_gesture_check(uint32_t button, uint32_t mods);

#endif
