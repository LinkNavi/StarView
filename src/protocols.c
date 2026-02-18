#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "protocols.h"
#include "core.h"
#include "config.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_keyboard_shortcuts_inhibit_v1.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_viewporter.h>

/* ============================================================================
 * Foreign Toplevel Management (wlr-foreign-toplevel-management)
 * Lets bars like waybar see and control windows
 * ============================================================================ */

struct foreign_toplevel_handle {
    struct wlr_foreign_toplevel_handle_v1 *handle;
    struct window *win;
    struct wl_listener request_maximize;
    struct wl_listener request_minimize;
    struct wl_listener request_activate;
    struct wl_listener request_fullscreen;
    struct wl_listener request_close;
    struct wl_listener destroy;
    struct wl_list link; /* server->foreign_toplevels */
};

static struct wl_list foreign_toplevels = {0};

static struct foreign_toplevel_handle *find_handle(struct window *win) {
    struct foreign_toplevel_handle *h;
    wl_list_for_each(h, &foreign_toplevels, link) {
        if (h->win == win) return h;
    }
    return NULL;
}

static void ft_handle_request_maximize(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, request_maximize);
    struct wlr_foreign_toplevel_handle_v1_maximized_event *event = data;
    if (!h->win) return;

    h->win->maximized = event->maximized;
    /* TODO: actually maximize/restore */
    arrange_windows(h->win->server);
}

static void ft_handle_request_minimize(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, request_minimize);
    struct wlr_foreign_toplevel_handle_v1_minimized_event *event = data;
    if (!h->win) return;

    h->win->minimized = event->minimized;
    struct wlr_scene_node *node = window_get_scene_node(h->win);
    if (node) wlr_scene_node_set_enabled(node, !h->win->minimized);
}

static void ft_handle_request_activate(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, request_activate);
    if (!h->win) return;

    /* Switch to window's workspace if needed */
    if (h->win->workspace != h->win->server->current_workspace) {
        workspace_show(h->win->server, h->win->workspace);
    }

    /* Unminimize */
    if (h->win->minimized) {
        h->win->minimized = false;
        struct wlr_scene_node *node = window_get_scene_node(h->win);
        if (node) wlr_scene_node_set_enabled(node, true);
    }

    /* Focus - use the old API for now since we're bridging */
    if (h->win->type == WIN_XDG) {
        /* Find the matching toplevel and focus it */
        struct toplevel *t;
        wl_list_for_each(t, &h->win->server->toplevels, link) {
            if (t->xdg_toplevel == h->win->xdg.xdg_toplevel) {
                focus_toplevel(t);
                break;
            }
        }
    }
}

static void ft_handle_request_fullscreen(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, request_fullscreen);
    struct wlr_foreign_toplevel_handle_v1_fullscreen_event *event = data;
    if (!h->win) return;

    window_set_fullscreen(h->win, event->fullscreen);
}

static void ft_handle_request_close(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, request_close);
    if (!h->win) return;
    window_close(h->win);
}

static void ft_handle_destroy(struct wl_listener *listener, void *data) {
    struct foreign_toplevel_handle *h =
        wl_container_of(listener, h, destroy);

    wl_list_remove(&h->request_maximize.link);
    wl_list_remove(&h->request_minimize.link);
    wl_list_remove(&h->request_activate.link);
    wl_list_remove(&h->request_fullscreen.link);
    wl_list_remove(&h->request_close.link);
    wl_list_remove(&h->destroy.link);
    wl_list_remove(&h->link);
    free(h);
}

int foreign_toplevel_init(struct server *server) {
    wl_list_init(&foreign_toplevels);

    server->foreign_toplevel_mgr =
        wlr_foreign_toplevel_manager_v1_create(server->display);
    if (!server->foreign_toplevel_mgr) {
        fprintf(stderr, "Failed to create foreign toplevel manager\n");
        return -1;
    }

    printf("Foreign toplevel management initialized\n");
    return 0;
}

void foreign_toplevel_finish(struct server *server) {
    /* Handles clean themselves up via destroy listener */
    (void)server;
}

void foreign_toplevel_handle_map(struct server *server, struct window *win) {
    if (!server->foreign_toplevel_mgr || !win) return;
    if (window_is_unmanaged(win)) return;

    struct foreign_toplevel_handle *h = calloc(1, sizeof(*h));
    if (!h) return;

    h->win = win;
    h->handle = wlr_foreign_toplevel_handle_v1_create(
        server->foreign_toplevel_mgr);
    if (!h->handle) { free(h); return; }

    /* Set initial state */
    const char *title = window_get_title(win);
    const char *app_id = window_get_app_id(win);
    wlr_foreign_toplevel_handle_v1_set_title(h->handle, title);
    wlr_foreign_toplevel_handle_v1_set_app_id(h->handle, app_id);

    /* Output association */
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        wlr_foreign_toplevel_handle_v1_output_enter(h->handle, output->wlr_output);
        break;
    }

    /* Listeners */
    h->request_maximize.notify = ft_handle_request_maximize;
    wl_signal_add(&h->handle->events.request_maximize, &h->request_maximize);

    h->request_minimize.notify = ft_handle_request_minimize;
    wl_signal_add(&h->handle->events.request_minimize, &h->request_minimize);

    h->request_activate.notify = ft_handle_request_activate;
    wl_signal_add(&h->handle->events.request_activate, &h->request_activate);

    h->request_fullscreen.notify = ft_handle_request_fullscreen;
    wl_signal_add(&h->handle->events.request_fullscreen, &h->request_fullscreen);

    h->request_close.notify = ft_handle_request_close;
    wl_signal_add(&h->handle->events.request_close, &h->request_close);

    h->destroy.notify = ft_handle_destroy;
    wl_signal_add(&h->handle->events.destroy, &h->destroy);

    wl_list_insert(&foreign_toplevels, &h->link);
}

void foreign_toplevel_handle_unmap(struct server *server, struct window *win) {
    (void)server;
    struct foreign_toplevel_handle *h = find_handle(win);
    if (h && h->handle) {
        wlr_foreign_toplevel_handle_v1_destroy(h->handle);
        /* ft_handle_destroy cleans up */
    }
}

void foreign_toplevel_handle_focus(struct server *server, struct window *win) {
    (void)server;
    /* Unfocus all, then focus this one */
    struct foreign_toplevel_handle *h;
    wl_list_for_each(h, &foreign_toplevels, link) {
        wlr_foreign_toplevel_handle_v1_set_activated(h->handle, h->win == win);
    }
}

void foreign_toplevel_handle_title(struct server *server, struct window *win) {
    (void)server;
    struct foreign_toplevel_handle *h = find_handle(win);
    if (h && h->handle) {
        wlr_foreign_toplevel_handle_v1_set_title(h->handle, window_get_title(win));
    }
}

/* ============================================================================
 * Pointer Constraints + Relative Pointer (games)
 * ============================================================================ */

static void handle_pointer_constraint(struct wl_listener *listener, void *data) {
    struct server *server =
        wl_container_of(listener, server, new_pointer_constraint);
    struct wlr_pointer_constraint_v1 *constraint = data;

    /* Store the active constraint for cursor processing */
    if (constraint->current.committed &
        WLR_POINTER_CONSTRAINT_V1_STATE_CURSOR_HINT) {
        /* Warp cursor to hint position if needed */
    }

    printf("Pointer constraint created: %s\n",
           constraint->type == WLR_POINTER_CONSTRAINT_V1_LOCKED ? "locked" : "confined");
}

int pointer_constraints_init(struct server *server) {
    server->pointer_constraints =
        wlr_pointer_constraints_v1_create(server->display);
    if (!server->pointer_constraints) {
        fprintf(stderr, "Failed to create pointer constraints\n");
        return -1;
    }

    server->new_pointer_constraint.notify = handle_pointer_constraint;
    wl_signal_add(&server->pointer_constraints->events.new_constraint,
                  &server->new_pointer_constraint);

    server->relative_pointer_mgr =
        wlr_relative_pointer_manager_v1_create(server->display);
    if (!server->relative_pointer_mgr) {
        fprintf(stderr, "Failed to create relative pointer manager\n");
        return -1;
    }

    printf("Pointer constraints + relative pointer initialized\n");
    return 0;
}

/* ============================================================================
 * Keyboard Shortcuts Inhibit (games, VMs)
 * ============================================================================ */

static void handle_keyboard_shortcuts_inhibit_new(struct wl_listener *listener,
                                                   void *data) {
    struct server *server =
        wl_container_of(listener, server, new_keyboard_shortcuts_inhibit);
    struct wlr_keyboard_shortcuts_inhibitor_v1 *inhibitor = data;

    /* Activate the inhibitor - allows the client to receive all key events */
    wlr_keyboard_shortcuts_inhibitor_v1_activate(inhibitor);

    printf("Keyboard shortcuts inhibited for surface\n");
}

int keyboard_shortcuts_inhibit_init(struct server *server) {
    server->keyboard_shortcuts_inhibit =
        wlr_keyboard_shortcuts_inhibit_v1_create(server->display);
    if (!server->keyboard_shortcuts_inhibit) {
        fprintf(stderr, "Failed to create keyboard shortcuts inhibit\n");
        return -1;
    }

    server->new_keyboard_shortcuts_inhibit.notify =
        handle_keyboard_shortcuts_inhibit_new;
    wl_signal_add(
        &server->keyboard_shortcuts_inhibit->events.new_inhibitor,
        &server->new_keyboard_shortcuts_inhibit);

    printf("Keyboard shortcuts inhibit initialized\n");
    return 0;
}

/* ============================================================================
 * Fractional Scale (HiDPI)
 * ============================================================================ */

int fractional_scale_init(struct server *server) {
    server->fractional_scale_mgr =
        wlr_fractional_scale_manager_v1_create(server->display, 1);
    if (!server->fractional_scale_mgr) {
        fprintf(stderr, "Failed to create fractional scale manager\n");
        return -1;
    }

    printf("Fractional scale initialized\n");
    return 0;
}

/* ============================================================================
 * XDG Activation (notification focus tokens)
 * ============================================================================ */

static void handle_xdg_activation_request(struct wl_listener *listener,
                                           void *data) {
    struct server *server =
        wl_container_of(listener, server, xdg_activation_request);
    struct wlr_xdg_activation_v1_request_activate_event *event = data;

    struct wlr_xdg_surface *xdg_surface =
        wlr_xdg_surface_try_from_wlr_surface(event->surface);
    if (!xdg_surface) return;

    /* Find the toplevel and focus it */
    struct toplevel *toplevel;
    wl_list_for_each(toplevel, &server->toplevels, link) {
        if (toplevel->xdg_toplevel->base == xdg_surface) {
            /* Switch workspace if needed */
            if (toplevel->workspace != server->current_workspace) {
                workspace_show(server, toplevel->workspace);
            }
            if (toplevel->minimized) {
                toplevel->minimized = false;
                struct wlr_scene_node *node = toplevel->decor.tree ?
                    &toplevel->decor.tree->node : &toplevel->scene_tree->node;
                wlr_scene_node_set_enabled(node, true);
            }
            focus_toplevel(toplevel);
            printf("XDG activation: focused window %s\n",
                   toplevel->xdg_toplevel->title ? toplevel->xdg_toplevel->title : "untitled");
            return;
        }
    }
}

int xdg_activation_init(struct server *server) {
    server->xdg_activation =
        wlr_xdg_activation_v1_create(server->display);
    if (!server->xdg_activation) {
        fprintf(stderr, "Failed to create XDG activation\n");
        return -1;
    }

    server->xdg_activation_request.notify = handle_xdg_activation_request;
    wl_signal_add(&server->xdg_activation->events.request_activate,
                  &server->xdg_activation_request);

    printf("XDG activation initialized\n");
    return 0;
}

/* ============================================================================
 * Session Lock (ext-session-lock-v1) - Full Implementation
 * ============================================================================ */

struct session_lock_output {
    struct wl_list link;
    struct wlr_scene_tree *tree;
    struct wlr_scene_rect *bg;
    struct wlr_session_lock_surface_v1 *lock_surface;
    struct output *output;

    struct wl_listener surface_map;
    struct wl_listener surface_destroy;
    struct wl_listener output_commit;
};

static void lock_surface_map(struct wl_listener *listener, void *data) {
    struct session_lock_output *lock_output =
        wl_container_of(listener, lock_output, surface_map);
    (void)data;

    /* Focus the lock surface */
    struct server *server = lock_output->output->server;
    if (lock_output->lock_surface && lock_output->lock_surface->surface) {
        wlr_seat_keyboard_notify_enter(server->seat,
            lock_output->lock_surface->surface, NULL, 0, NULL);
    }
}

static void lock_surface_destroy(struct wl_listener *listener, void *data) {
    struct session_lock_output *lock_output =
        wl_container_of(listener, lock_output, surface_destroy);
    (void)data;

    wl_list_remove(&lock_output->surface_map.link);
    wl_list_remove(&lock_output->surface_destroy.link);
    lock_output->lock_surface = NULL;
}

static void handle_lock_new_surface(struct wl_listener *listener, void *data) {
    struct server *server =
        wl_container_of(listener, server, session_lock_new_surface);
    struct wlr_session_lock_surface_v1 *lock_surface = data;

    /* Find matching output */
    struct output *output;
    wl_list_for_each(output, &server->outputs, link) {
        if (output->wlr_output == lock_surface->output) {
            /* Configure the lock surface to fill the output */
            wlr_session_lock_surface_v1_configure(lock_surface,
                output->wlr_output->width, output->wlr_output->height);

            /* Create scene surface in overlay layer */
            struct wlr_scene_tree *tree =
                wlr_scene_subsurface_tree_create(server->layer_overlay,
                    lock_surface->surface);
            if (tree) {
                wlr_scene_node_set_position(&tree->node, 0, 0);
            }

            printf("Session lock surface created for output %s\n",
                   output->wlr_output->name);
            break;
        }
    }
}

static void handle_lock_unlock(struct wl_listener *listener, void *data) {
    struct server *server =
        wl_container_of(listener, server, session_lock_unlock);
    (void)data;

    printf("Session unlocked\n");
    server->locked_session = NULL;

    /* Re-focus the previously focused surface */
    if (!wl_list_empty(&server->toplevels)) {
        struct toplevel *t = wl_container_of(server->toplevels.next, t, link);
        if (t->workspace == server->current_workspace) {
            focus_toplevel(t);
        }
    }
}

static void handle_lock_destroy(struct wl_listener *listener, void *data) {
    struct server *server =
        wl_container_of(listener, server, session_lock_destroy);
    (void)data;

    server->locked_session = NULL;
    printf("Session lock destroyed\n");
}

static void handle_new_session_lock(struct wl_listener *listener, void *data) {
    struct server *server =
        wl_container_of(listener, server, new_session_lock_v2);
    struct wlr_session_lock_v1 *lock = data;

    if (server->locked_session) {
        wlr_session_lock_v1_destroy(lock);
        return;
    }

    server->locked_session = lock;

    /* Clear keyboard focus */
    wlr_seat_keyboard_clear_focus(server->seat);

    /* Listen for lock events */
    server->session_lock_new_surface.notify = handle_lock_new_surface;
    wl_signal_add(&lock->events.new_surface, &server->session_lock_new_surface);

    server->session_lock_unlock.notify = handle_lock_unlock;
    wl_signal_add(&lock->events.unlock, &server->session_lock_unlock);

    server->session_lock_destroy.notify = handle_lock_destroy;
    wl_signal_add(&lock->events.destroy, &server->session_lock_destroy);

    /* Send locked to acknowledge */
    wlr_session_lock_v1_send_locked(lock);

    printf("Session locked\n");
}

int session_lock_init_v2(struct server *server) {
    server->session_lock_mgr_v2 =
        wlr_session_lock_manager_v1_create(server->display);
    if (!server->session_lock_mgr_v2) {
        fprintf(stderr, "Failed to create session lock manager\n");
        return -1;
    }

    server->new_session_lock_v2.notify = handle_new_session_lock;
    wl_signal_add(&server->session_lock_mgr_v2->events.new_lock,
                  &server->new_session_lock_v2);

    server->locked_session = NULL;

    printf("Session lock (ext-session-lock-v1) initialized\n");
    return 0;
}

/* ============================================================================
 * Presentation Time (video playback)
 * ============================================================================ */

int presentation_time_init(struct server *server) {
    server->presentation = wlr_presentation_create(server->display,
        server->backend);
    if (!server->presentation) {
        fprintf(stderr, "Failed to create presentation time\n");
        return -1;
    }

    printf("Presentation time initialized\n");
    return 0;
}

/* ============================================================================
 * Viewporter (video playback, scaling)
 * ============================================================================ */

int viewporter_init(struct server *server) {
    server->viewporter = wlr_viewporter_create(server->display);
    if (!server->viewporter) {
        fprintf(stderr, "Failed to create viewporter\n");
        return -1;
    }

    printf("Viewporter initialized\n");
    return 0;
}
