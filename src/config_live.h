#ifndef CONFIG_LIVE_H
#define CONFIG_LIVE_H

struct server;

/* Apply config changes live after config_reload(). */
void config_apply_live(struct server *server);

#endif
