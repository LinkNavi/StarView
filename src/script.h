#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdbool.h>
#include <stdint.h>

/* Forward declarations */
struct server;
struct toplevel;
struct output;

/* Script engine handle */
typedef struct script_engine script_engine_t;

/* Script callback types */
typedef void (*script_window_callback)(struct toplevel *toplevel);
typedef void (*script_workspace_callback)(int workspace);
typedef void (*script_output_callback)(struct output *output);

/* Initialize the script engine */
script_engine_t *script_engine_init(struct server *server);

/* Cleanup the script engine */
void script_engine_cleanup(script_engine_t *engine);

/* Load and execute a script file */
int script_load_file(script_engine_t *engine, const char *path);

/* Execute a script string */
int script_execute(script_engine_t *engine, const char *code);

/* Call a script function by name */
int script_call_function(script_engine_t *engine, const char *func_name);

/* Register event hooks */
void script_on_window_open(script_engine_t *engine, const char *func_name);
void script_on_window_close(script_engine_t *engine, const char *func_name);
void script_on_window_focus(script_engine_t *engine, const char *func_name);
void script_on_workspace_switch(script_engine_t *engine, const char *func_name);
void script_on_output_add(script_engine_t *engine, const char *func_name);

/* Trigger events from compositor */
void script_trigger_window_open(script_engine_t *engine, struct toplevel *toplevel);
void script_trigger_window_close(script_engine_t *engine, struct toplevel *toplevel);
void script_trigger_window_focus(script_engine_t *engine, struct toplevel *toplevel);
void script_trigger_workspace_switch(script_engine_t *engine, int workspace);
void script_trigger_output_add(script_engine_t *engine, struct output *output);

/* Hot reload support */
int script_reload(script_engine_t *engine);

#endif /* SCRIPT_H */
