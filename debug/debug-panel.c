// debug-panel.c
// Compile: gcc debug-panel.c -o debug-panel $(pkg-config --cflags --libs gtk4 gtk4-layer-shell-0)

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Debug Panel");
    
    // Initialize layer shell
    gtk_layer_init_for_window(GTK_WINDOW(window));
    
    // Set layer to TOP
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_TOP);
    
    // Anchor to top edge and left/right sides
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE); // NOT bottom!
    
    // No margins
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    
    // CRITICAL: Set exclusive zone to reserve space
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 40);
    
    // Don't need keyboard
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), 
        GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    
    g_print("=== Panel Configuration ===\n");
    g_print("Layer: TOP\n");
    g_print("Anchors: TOP + LEFT + RIGHT (NOT BOTTOM)\n");
    g_print("Exclusive zone: 40px\n");
    g_print("Expected: 40px tall bar at top of screen\n");
    
    // Simple label
    GtkWidget *label = gtk_label_new("40px Panel - Should be at TOP");
    gtk_widget_set_margin_start(label, 10);
    gtk_widget_set_margin_end(label, 10);
    gtk_widget_set_margin_top(label, 10);
    gtk_widget_set_margin_bottom(label, 10);
    
    // Apply CSS to make it obvious
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider,
        "window {"
        "  background-color: rgba(255, 0, 0, 0.8);"  // RED so it's obvious
        "}"
        "label {"
        "  color: white;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
    );
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    gtk_window_set_child(GTK_WINDOW(window), label);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new(
        "com.starview.debugpanel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
