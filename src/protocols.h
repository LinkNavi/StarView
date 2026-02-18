#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#define _POSIX_C_SOURCE 200809L

#include <wayland-server-core.h>

struct server;

/* Foreign toplevel management (waybar window list) */
int foreign_toplevel_init(struct server *server);
void foreign_toplevel_finish(struct server *server);
void foreign_toplevel_handle_map(struct server *server, struct window *win);
void foreign_toplevel_handle_unmap(struct server *server, struct window *win);
void foreign_toplevel_handle_focus(struct server *server, struct window *win);
void foreign_toplevel_handle_title(struct server *server, struct window *win);

/* Pointer constraints + relative pointer (games) */
int pointer_constraints_init(struct server *server);

/* Keyboard shortcuts inhibit (games/VMs) */
int keyboard_shortcuts_inhibit_init(struct server *server);

/* Fractional scale (HiDPI) */
int fractional_scale_init(struct server *server);

/* XDG activation (notification focus tokens) */
int xdg_activation_init(struct server *server);

/* Session lock (screen locker - full implementation) */
int session_lock_init_v2(struct server *server);

/* Presentation time + viewporter (video playback) */
int presentation_time_init(struct server *server);
int viewporter_init(struct server *server);

#endif /* PROTOCOLS_H */
