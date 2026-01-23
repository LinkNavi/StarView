#define _POSIX_C_SOURCE 200809L
#define WLR_USE_UNSTABLE

#include "script.h"
#include "core.h"
#include "config.h"
#include <magpie.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Script engine state */
struct script_engine {
    ScriptEngine *magpie_engine;
    struct server *server;
    
    /* Event callback names */
    char *on_window_open;
    char *on_window_close;
    char *on_window_focus;
    char *on_workspace_switch;
    char *on_output_add;
    
    /* Script file path for hot reload */
    char *script_path;
};

/* Native function: log(message: string) */
static Val native_log(const Val *args) {
    if (args[0].tag != VAL_STR) {
        fprintf(stderr, "[Script Error] log() expects string argument\n");
        return (Val){.tag = VAL_NULL};
    }
    printf("[Script] %s\n", args[0].u.s->data);
    return (Val){.tag = VAL_NULL};
}

/* Native function: get_workspace() -> int */
static Val native_get_workspace(const Val *args) {
    (void)args;
    /* Get from global server context */
    extern struct server g_server;
    return (Val){.tag = VAL_INT, .u.i = g_server.current_workspace};
}

/* Native function: set_workspace(workspace: int) */
static Val native_set_workspace(const Val *args) {
    if (args[0].tag != VAL_INT) {
        fprintf(stderr, "[Script Error] set_workspace() expects int argument\n");
        return (Val){.tag = VAL_NULL};
    }
    
    extern struct server g_server;
    int ws = (int)args[0].u.i;
    if (ws >= 1 && ws <= MAX_WORKSPACES) {
        workspace_show(&g_server, ws);
    }
    return (Val){.tag = VAL_NULL};
}

/* Native function: get_focused_window() -> object */
static Val native_get_focused_window(const Val *args) {
    (void)args;
    extern struct server g_server;
    
    struct toplevel *focused = get_focused_toplevel(&g_server);
    if (!focused) {
        return (Val){.tag = VAL_NULL};
    }
    
    /* Create object with window info */
    Obj *obj = obj_new();
    
    const char *title = focused->xdg_toplevel->title;
    const char *app_id = focused->xdg_toplevel->app_id;
    
    if (title) {
        obj_set(obj, "title", (Val){.tag = VAL_STR, .u.s = str_new(title)});
    }
    if (app_id) {
        obj_set(obj, "app_id", (Val){.tag = VAL_STR, .u.s = str_new(app_id)});
    }
    
    obj_set(obj, "workspace", (Val){.tag = VAL_INT, .u.i = focused->workspace});
    obj_set(obj, "floating", (Val){.tag = VAL_BOOL, .u.b = focused->floating});
    obj_set(obj, "fullscreen", (Val){.tag = VAL_BOOL, .u.b = focused->fullscreen});
    
    return (Val){.tag = VAL_OBJ, .u.o = obj};
}

/* Native function: close_window() */
static Val native_close_window(const Val *args) {
    (void)args;
    extern struct server g_server;
    
    struct toplevel *focused = get_focused_toplevel(&g_server);
    if (focused) {
        wlr_xdg_toplevel_send_close(focused->xdg_toplevel);
    }
    return (Val){.tag = VAL_NULL};
}

/* Native function: toggle_floating() */
static Val native_toggle_floating(const Val *args) {
    (void)args;
    extern struct server g_server;
    
    struct toplevel *focused = get_focused_toplevel(&g_server);
    if (focused) {
        focused->floating = !focused->floating;
        arrange_windows(&g_server);
    }
    return (Val){.tag = VAL_NULL};
}

/* Native function: spawn(command: string) */
static Val native_spawn(const Val *args) {
    if (args[0].tag != VAL_STR) {
        fprintf(stderr, "[Script Error] spawn() expects string argument\n");
        return (Val){.tag = VAL_NULL};
    }
    
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", args[0].u.s->data, NULL);
        _exit(1);
    }
    return (Val){.tag = VAL_NULL};
}

/* Native function: get_output_count() -> int */
static Val native_get_output_count(const Val *args) {
    (void)args;
    extern struct server g_server;
    
    int count = 0;
    struct output *output;
    wl_list_for_each(output, &g_server.outputs, link) {
        count++;
    }
    return (Val){.tag = VAL_INT, .u.i = count};
}

/* Native function: reload_config() */
static Val native_reload_config(const Val *args) {
    (void)args;
    extern struct server g_server;
    
    config_reload();
    arrange_windows(&g_server);
    return (Val){.tag = VAL_NULL};
}

/* Native function: notify(title: string, message: string) */
static Val native_notify(const Val *args) {
    if (args[0].tag != VAL_STR || args[1].tag != VAL_STR) {
        fprintf(stderr, "[Script Error] notify() expects two string arguments\n");
        return (Val){.tag = VAL_NULL};
    }
    
    /* Use notify-send if available */
    if (fork() == 0) {
        execl("/usr/bin/notify-send", "notify-send", 
              args[0].u.s->data, args[1].u.s->data, NULL);
        _exit(1);
    }
    return (Val){.tag = VAL_NULL};
}

/* Native function: set_gaps(inner: int, outer: int) */
static Val native_set_gaps(const Val *args) {
    if (args[0].tag != VAL_INT || args[1].tag != VAL_INT) {
        fprintf(stderr, "[Script Error] set_gaps() expects two int arguments\n");
        return (Val){.tag = VAL_NULL};
    }
    
    extern struct server g_server;
    config.gaps_inner = (int)args[0].u.i;
    config.gaps_outer = (int)args[1].u.i;
    arrange_windows(&g_server);
    return (Val){.tag = VAL_NULL};
}

/* Initialize script engine */
script_engine_t *script_engine_init(struct server *server) {
    script_engine_t *engine = calloc(1, sizeof(*engine));
    if (!engine) return NULL;
    
    engine->server = server;
    engine->magpie_engine = ScriptEngine_new();
    if (!engine->magpie_engine) {
        free(engine);
        return NULL;
    }
    
    /* Register native functions */
    ScriptEngine_register_native(engine->magpie_engine, "log", native_log);
    ScriptEngine_register_native(engine->magpie_engine, "get_workspace", native_get_workspace);
    ScriptEngine_register_native(engine->magpie_engine, "set_workspace", native_set_workspace);
    ScriptEngine_register_native(engine->magpie_engine, "get_focused_window", native_get_focused_window);
    ScriptEngine_register_native(engine->magpie_engine, "close_window", native_close_window);
    ScriptEngine_register_native(engine->magpie_engine, "toggle_floating", native_toggle_floating);
    ScriptEngine_register_native(engine->magpie_engine, "spawn", native_spawn);
    ScriptEngine_register_native(engine->magpie_engine, "get_output_count", native_get_output_count);
    ScriptEngine_register_native(engine->magpie_engine, "reload_config", native_reload_config);
    ScriptEngine_register_native(engine->magpie_engine, "notify", native_notify);
    ScriptEngine_register_native(engine->magpie_engine, "set_gaps", native_set_gaps);
    
    printf("[Script] Engine initialized with Magpie runtime\n");
    return engine;
}

/* Cleanup script engine */
void script_engine_cleanup(script_engine_t *engine) {
    if (!engine) return;
    
    free(engine->on_window_open);
    free(engine->on_window_close);
    free(engine->on_window_focus);
    free(engine->on_workspace_switch);
    free(engine->on_output_add);
    free(engine->script_path);
    
    ScriptEngine_free(engine->magpie_engine);
    free(engine);
}

/* Load and execute a script file */
int script_load_file(script_engine_t *engine, const char *path) {
    if (!engine || !path) return -1;
    
    /* Read file */
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[Script Error] Failed to open: %s\n", path);
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *code = malloc(size + 1);
    if (!code) {
        fclose(fp);
        return -1;
    }
    
    fread(code, 1, size, fp);
    code[size] = '\0';
    fclose(fp);
    
    /* Save path for reload */
    free(engine->script_path);
    engine->script_path = strdup(path);
    
    /* Compile and run */
    int ret = ScriptEngine_compile(engine->magpie_engine, "user_script", code);
    if (ret == 0) {
        ret = ScriptEngine_run(engine->magpie_engine, "user_script");
    }
    
    free(code);
    
    if (ret != 0) {
        fprintf(stderr, "[Script Error] Failed to load script: %s\n", path);
        return -1;
    }
    
    printf("[Script] Loaded: %s\n", path);
    return 0;
}

/* Execute a script string */
int script_execute(script_engine_t *engine, const char *code) {
    if (!engine || !code) return -1;
    
    int ret = ScriptEngine_compile(engine->magpie_engine, "inline", code);
    if (ret == 0) {
        ret = ScriptEngine_run(engine->magpie_engine, "inline");
    }
    
    if (ret != 0) {
        fprintf(stderr, "[Script Error] Failed to execute code\n");
        return -1;
    }
    
    return 0;
}

/* Call a script function by name */
int script_call_function(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return -1;
    
    /* Construct call code */
    char call_code[256];
    snprintf(call_code, sizeof(call_code), "%s();", func_name);
    
    return script_execute(engine, call_code);
}

/* Register event hooks */
void script_on_window_open(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return;
    free(engine->on_window_open);
    engine->on_window_open = strdup(func_name);
}

void script_on_window_close(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return;
    free(engine->on_window_close);
    engine->on_window_close = strdup(func_name);
}

void script_on_window_focus(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return;
    free(engine->on_window_focus);
    engine->on_window_focus = strdup(func_name);
}

void script_on_workspace_switch(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return;
    free(engine->on_workspace_switch);
    engine->on_workspace_switch = strdup(func_name);
}

void script_on_output_add(script_engine_t *engine, const char *func_name) {
    if (!engine || !func_name) return;
    free(engine->on_output_add);
    engine->on_output_add = strdup(func_name);
}

/* Trigger events */
void script_trigger_window_open(script_engine_t *engine, struct toplevel *toplevel) {
    if (!engine || !engine->on_window_open || !toplevel) return;
    
    /* Set global context */
    const char *title = toplevel->xdg_toplevel->title;
    const char *app_id = toplevel->xdg_toplevel->app_id;
    
    char code[512];
    snprintf(code, sizeof(code), 
             "let var window = {title: \"%s\", app_id: \"%s\", workspace: %d}; %s(window);",
             title ? title : "Untitled",
             app_id ? app_id : "unknown",
             toplevel->workspace,
             engine->on_window_open);
    
    script_execute(engine, code);
}

void script_trigger_window_close(script_engine_t *engine, struct toplevel *toplevel) {
    if (!engine || !engine->on_window_close || !toplevel) return;
    script_call_function(engine, engine->on_window_close);
}

void script_trigger_window_focus(script_engine_t *engine, struct toplevel *toplevel) {
    if (!engine || !engine->on_window_focus || !toplevel) return;
    script_call_function(engine, engine->on_window_focus);
}

void script_trigger_workspace_switch(script_engine_t *engine, int workspace) {
    if (!engine || !engine->on_workspace_switch) return;
    
    char code[128];
    snprintf(code, sizeof(code), "%s(%d);", engine->on_workspace_switch, workspace);
    script_execute(engine, code);
}

void script_trigger_output_add(script_engine_t *engine, struct output *output) {
    if (!engine || !engine->on_output_add || !output) return;
    script_call_function(engine, engine->on_output_add);
}

/* Hot reload support */
int script_reload(script_engine_t *engine) {
    if (!engine || !engine->script_path) return -1;
    
    printf("[Script] Reloading: %s\n", engine->script_path);
    return script_load_file(engine, engine->script_path);
}
