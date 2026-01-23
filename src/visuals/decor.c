
#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "../config.h"
#include "../core.h"
#include "../titlebar_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_scene.h>

/* Global theme - managed by main.c */
static struct titlebar_theme *g_global_theme = NULL;

/* Color conversion helper */
static void color_to_float(uint32_t color, float out[4]) {
  out[0] = ((color >> 24) & 0xff) / 255.0f;
  out[1] = ((color >> 16) & 0xff) / 255.0f;
  out[2] = ((color >> 8) & 0xff) / 255.0f;
  out[3] = (color & 0xff) / 255.0f;
}

/* Set global theme (called from main.c) */
void decor_set_global_theme(struct titlebar_theme *theme) {
  g_global_theme = theme;
  fprintf(stderr, "[DECOR] Global theme set: %p\n", (void *)theme);
  if (theme) {
    fprintf(stderr, "[DECOR]   height=%d, bg_color=r:%.2f g:%.2f b:%.2f\n",
            theme->height, theme->bg_color.r, theme->bg_color.g,
            theme->bg_color.b);
  }
}

/* Get global theme */
struct titlebar_theme *decor_get_global_theme(void) { return g_global_theme; }

void decor_create(struct toplevel *toplevel) {
  fprintf(stderr, "[DECOR] decor_create() called for window\n");
  fprintf(stderr, "[DECOR]   config.decor.enabled = %d\n",
          config.decor.enabled);
  fprintf(stderr, "[DECOR]   global_theme = %p\n", (void *)g_global_theme);

  if (!config.decor.enabled) {
    fprintf(stderr, "[DECOR]   SKIPPED: decorations disabled in config\n");
    return;
  }

  if (!g_global_theme) {
    fprintf(
        stderr,
        "[DECOR]   WARNING: No global theme set! Creating default theme.\n");
    g_global_theme = titlebar_theme_create();
    if (g_global_theme) {
      titlebar_theme_load_from_config(g_global_theme, &config.decor);
      fprintf(stderr, "[DECOR]   Created default theme: height=%d\n",
              g_global_theme->height);
    } else {
      fprintf(stderr, "[DECOR]   ERROR: Failed to create default theme!\n");
      return;
    }
  }

  struct server *server = toplevel->server;
  struct decoration *d = &toplevel->decor;

  int h = g_global_theme->height;
  int bw = config.border_width;

  fprintf(stderr, "[DECOR]   Creating decoration tree (height=%d, border=%d)\n",
          h, bw);

  /* Create decoration tree as parent of window */
  d->tree = wlr_scene_tree_create(server->layer_windows);
  if (!d->tree) {
    fprintf(stderr, "[DECOR]   ERROR: Failed to create scene tree!\n");
    return;
  }

  /* Reparent toplevel scene tree under decoration tree FIRST */
  wlr_scene_node_reparent(&toplevel->scene_tree->node, d->tree);
  wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, h);

  /* Get geometry for shadow sizing */
  struct wlr_box geo;
  wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
  int window_width = geo.width > 0 ? geo.width : 800;
  int window_height = geo.height > 0 ? geo.height : 600;

  /* Create shadow as SIBLING to decoration tree, not child */
  if (config.decor.shadow.enabled) {
    d->shadow = create_shadow_buffer(server->layer_windows, window_width,
                                     window_height + h, &config.decor.shadow);
    if (d->shadow) {
      /* Lower shadow below everything */
      wlr_scene_node_lower_to_bottom(&d->shadow->node);
      /* Shadow will be positioned in decor_set_size */
    }
  }

  /* Create Cairo-rendered titlebar */
  fprintf(stderr, "[DECOR]   Creating rendered titlebar...\n");
  d->rendered_titlebar = titlebar_render_create(d->tree, g_global_theme);
  if (!d->rendered_titlebar) {
    fprintf(stderr, "[DECOR]   ERROR: Failed to create rendered titlebar!\n");
    if (d->shadow) {
      wlr_scene_node_destroy(&d->shadow->node);
      d->shadow = NULL;
    }
    wlr_scene_node_destroy(&d->tree->node);
    d->tree = NULL;
    return;
  }

  /* Create borders using scene rects */
  float border_color[4];
  color_to_float(config.border_color_active, border_color);

  d->border_top = wlr_scene_rect_create(d->tree, 100, bw, border_color);
  d->border_bottom = wlr_scene_rect_create(d->tree, 100, bw, border_color);
  d->border_left = wlr_scene_rect_create(d->tree, bw, 100, border_color);
  d->border_right = wlr_scene_rect_create(d->tree, bw, 100, border_color);

  if (!d->border_top || !d->border_bottom || !d->border_left ||
      !d->border_right) {
    fprintf(stderr, "[DECOR]   ERROR: Failed to create borders!\n");
    if (d->rendered_titlebar)
      titlebar_render_destroy(d->rendered_titlebar);
    if (d->shadow) {
      wlr_scene_node_destroy(&d->shadow->node);
      d->shadow = NULL;
    }
    wlr_scene_node_destroy(&d->tree->node);
    d->tree = NULL;
    d->rendered_titlebar = NULL;
    return;
  }

  d->width = 800; /* Will be updated by decor_set_size */

  fprintf(stderr, "[DECOR]   ✓ Decoration created successfully!\n");
}

void decor_set_size(struct toplevel *toplevel, int width) {
  if (!config.decor.enabled || !toplevel->decor.tree) {
    return;
  }

  if (!g_global_theme) {
    fprintf(stderr, "[DECOR] WARNING: No global theme in decor_set_size!\n");
    return;
  }

  struct decoration *d = &toplevel->decor;

  int h = g_global_theme->height;
  int bw = config.border_width;

  d->width = width;

  /* Update Cairo titlebar */
  if (d->rendered_titlebar) {
    const char *title = toplevel->xdg_toplevel->title;
    if (!title)
      title = toplevel->xdg_toplevel->app_id;
    if (!title)
      title = "Untitled";

    bool active = (toplevel == get_focused_toplevel(toplevel->server));

    titlebar_render_update(d->rendered_titlebar, g_global_theme, width, title,
                           active);
  }

  /* Recreate shadow only if size changed */
  if (config.decor.shadow.enabled) {
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int content_height = geo.height > 0 ? geo.height : 100;
    int total_height = h + content_height;

    // Check if shadow needs recreation
    if (!d->shadow || d->cached_shadow_width != width ||
        d->cached_shadow_height != total_height) {

      // Remove old shadow
      if (d->shadow) {
        wlr_scene_node_destroy(&d->shadow->node);
        d->shadow = NULL;
      }

      // Create new shadow
      d->shadow = create_shadow_buffer(toplevel->server->layer_windows, width,
                                       total_height, &config.decor.shadow);

      // Cache the size
      d->cached_shadow_width = width;
      d->cached_shadow_height = total_height;
    }

    // Update shadow position
    if (d->shadow) {
      int decor_x = d->tree->node.x;
      int decor_y = d->tree->node.y;
      wlr_scene_node_set_position(&d->shadow->node, decor_x, decor_y);
      wlr_scene_node_lower_to_bottom(&d->shadow->node);
    }
  }

  if (config.decor.shadow.enabled) {
    struct wlr_box geo;
    wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
    int content_height = geo.height > 0 ? geo.height : 100;
    int total_height = h + content_height;

    d->shadow = create_shadow_buffer(toplevel->server->layer_windows, width,
                                     total_height, &config.decor.shadow);
    if (d->shadow) {
      /* Position shadow to match decoration tree position */
      int decor_x = d->tree->node.x;
      int decor_y = d->tree->node.y;
      wlr_scene_node_set_position(&d->shadow->node, decor_x, decor_y);
      wlr_scene_node_lower_to_bottom(&d->shadow->node);
    }
  }

  /* Resize borders */
  struct wlr_box geo;
  wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
  int content_height = geo.height > 0 ? geo.height : 100;
  int total_height = h + content_height;

  if (d->border_top) {
    wlr_scene_rect_set_size(d->border_top, width + bw * 2, bw);
    wlr_scene_node_set_position(&d->border_top->node, -bw, -bw);
  }

  if (d->border_bottom) {
    wlr_scene_rect_set_size(d->border_bottom, width + bw * 2, bw);
    wlr_scene_node_set_position(&d->border_bottom->node, -bw, total_height);
  }

  if (d->border_left) {
    wlr_scene_rect_set_size(d->border_left, bw, total_height + bw * 2);
    wlr_scene_node_set_position(&d->border_left->node, -bw, -bw);
  }

  if (d->border_right) {
    wlr_scene_rect_set_size(d->border_right, bw, total_height + bw * 2);
    wlr_scene_node_set_position(&d->border_right->node, width, -bw);
  }
}
/* Update decoration (focus state) */
void decor_update(struct toplevel *toplevel, bool focused) {
  if (!config.decor.enabled || !toplevel->decor.tree) {
    return;
  }

  if (!g_global_theme) {
    return;
  }

  struct decoration *d = &toplevel->decor;

  /* Update Cairo titlebar for focus state */
  if (d->rendered_titlebar) {
    const char *title = toplevel->xdg_toplevel->title;
    if (!title)
      title = toplevel->xdg_toplevel->app_id;
    if (!title)
      title = "Untitled";

    titlebar_render_update(d->rendered_titlebar, g_global_theme, d->width,
                           title, focused);
  }

  /* Update border colors */
  float border[4];
  if (focused) {
    color_to_float(config.border_color_active, border);
  } else {
    color_to_float(config.border_color_inactive, border);
  }

  if (d->border_top)
    wlr_scene_rect_set_color(d->border_top, border);
  if (d->border_bottom)
    wlr_scene_rect_set_color(d->border_bottom, border);
  if (d->border_left)
    wlr_scene_rect_set_color(d->border_left, border);
  if (d->border_right)
    wlr_scene_rect_set_color(d->border_right, border);
}

/* Destroy decoration */
void decor_destroy(struct toplevel *toplevel) {
  if (!toplevel->decor.tree) {
    return;
  }

  if (toplevel->decor.rendered_titlebar) {
    titlebar_render_destroy(toplevel->decor.rendered_titlebar);
    toplevel->decor.rendered_titlebar = NULL;
  }
  if (toplevel->decor.shadow) {
    wlr_scene_node_destroy(&toplevel->decor.shadow->node);
    toplevel->decor.shadow = NULL;
  }
  wlr_scene_node_destroy(&toplevel->decor.tree->node);
  toplevel->decor.tree = NULL;
  toplevel->decor.border_top = NULL;
  toplevel->decor.border_bottom = NULL;
  toplevel->decor.border_left = NULL;
  toplevel->decor.border_right = NULL;
}

/* Hit test for decoration */
enum decor_hit decor_hit_test(struct toplevel *toplevel, double lx, double ly) {
  if (!config.decor.enabled || !toplevel->decor.tree) {
    return HIT_NONE;
  }

  if (!g_global_theme) {
    return HIT_NONE;
  }

  struct decoration *d = &toplevel->decor;

  /* Get decoration position */
  int dx = d->tree->node.x;
  int dy = d->tree->node.y;

  /* Local coordinates */
  double x = lx - dx;
  double y = ly - dy;

  int h = g_global_theme->height;
  int bw = config.border_width;
  int width = d->width;

  struct wlr_box geo;
  wlr_xdg_surface_get_geometry(toplevel->xdg_toplevel->base, &geo);
  int total_height = h + geo.height;

  /* Check resize edges first */
  int edge_size = 8;

  /* Corners */
  if (x < edge_size && y < edge_size)
    return HIT_RESIZE_TOP_LEFT;
  if (x >= width - edge_size && y < edge_size)
    return HIT_RESIZE_TOP_RIGHT;
  if (x < edge_size && y >= total_height - edge_size)
    return HIT_RESIZE_BOTTOM_LEFT;
  if (x >= width - edge_size && y >= total_height - edge_size)
    return HIT_RESIZE_BOTTOM_RIGHT;

  /* Edges */
  if (y < bw)
    return HIT_RESIZE_TOP;
  if (y >= total_height - bw)
    return HIT_RESIZE_BOTTOM;
  if (x < bw)
    return HIT_RESIZE_LEFT;
  if (x >= width - bw)
    return HIT_RESIZE_RIGHT;

  /* Titlebar area - use Cairo titlebar hit testing */
  if (y >= 0 && y < h && d->rendered_titlebar) {
    enum button_type btn =
        titlebar_render_hit_test(d->rendered_titlebar, (int)x, (int)y);

    switch (btn) {
    case BTN_TYPE_CLOSE:
      return HIT_CLOSE;
    case BTN_TYPE_MAXIMIZE:
      return HIT_MAXIMIZE;
    case BTN_TYPE_MINIMIZE:
      return HIT_MINIMIZE;
    default:
      return HIT_TITLEBAR;
    }
  }

  return HIT_NONE;
}

/* Update hover state */
void decor_update_hover(struct toplevel *toplevel, double lx, double ly) {
  if (!config.decor.enabled || !toplevel->decor.tree || !g_global_theme) {
    return;
  }

  struct decoration *d = &toplevel->decor;

  if (!d->rendered_titlebar) {
    return;
  }

  /* Get local coordinates */
  int dx = d->tree->node.x;
  int dy = d->tree->node.y;
  int x = (int)(lx - dx);
  int y = (int)(ly - dy);

  int h = g_global_theme->height;

  /* Reset all to normal first */
  enum button_state close_state = BTN_STATE_NORMAL;
  enum button_state max_state = BTN_STATE_NORMAL;
  enum button_state min_state = BTN_STATE_NORMAL;

  /* Check if in titlebar */
  if (y >= 0 && y < h) {
    enum button_type btn = titlebar_render_hit_test(d->rendered_titlebar, x, y);

    switch (btn) {
    case BTN_TYPE_CLOSE:
      close_state = BTN_STATE_HOVER;
      break;
    case BTN_TYPE_MAXIMIZE:
      max_state = BTN_STATE_HOVER;
      break;
    case BTN_TYPE_MINIMIZE:
      min_state = BTN_STATE_HOVER;
      break;
    default:
      break;
    }
  }

  /* Update button states if changed */
  if (d->hovered_close != (close_state == BTN_STATE_HOVER)) {
    d->hovered_close = (close_state == BTN_STATE_HOVER);
    titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_CLOSE,
                                     close_state);
  }
  if (d->hovered_max != (max_state == BTN_STATE_HOVER)) {
    d->hovered_max = (max_state == BTN_STATE_HOVER);
    titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_MAXIMIZE,
                                     max_state);
  }
  if (d->hovered_min != (min_state == BTN_STATE_HOVER)) {
    d->hovered_min = (min_state == BTN_STATE_HOVER);
    titlebar_render_set_button_state(d->rendered_titlebar, BTN_TYPE_MINIMIZE,
                                     min_state);
  }
}
