#include <adwaita.h>
#include "playlist.h"
#include "playback.h"
#include "audio_stream.h"

static void on_close(GtkApplication* app);
static void on_preferences_action(GSimpleAction*, GVariant*, gpointer);
static void on_about_action(GSimpleAction*, GVariant*, gpointer);

static const GActionEntry app_actions[] =
{
	{ "preferences", on_preferences_action, NULL, NULL, NULL, { 0 } },
	{ "about", on_about_action, NULL, NULL, NULL, { 0 } }
};

static void on_activate(GtkApplication* app)
{
    init_audio();

    // Create window
    GtkBuilder* builder = gtk_builder_new_from_resource("/com/github/lukawarren/waveform/src/ui/window.ui");
    GObject* window = gtk_builder_get_object(builder, "window");
    gtk_window_set_application(GTK_WINDOW(window), app);

    // Setup main menu
    g_action_map_add_action_entries(
        G_ACTION_MAP(app),
        app_actions,
        G_N_ELEMENTS(app_actions),
        window
    );
    gtk_application_set_accels_for_action(
        app,
        "app.preferences",
        (const char*[]) { "<primary>comma", NULL }
    );

    // Load CSS
    GtkCssProvider* css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(css_provider, "/com/github/lukawarren/waveform/src/ui/style.css");
    gtk_style_context_add_provider_for_display(
        gtk_widget_get_display(GTK_WIDGET(window)),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(css_provider);

    // Init tab screens
    init_playlist_ui(builder, GTK_WINDOW(window));
    init_playback_ui(builder);

    // Show window
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
    g_object_unref(builder);

    // Memory clean-up
    g_signal_connect(window, "destroy", G_CALLBACK(on_close), NULL);
}

static void on_close(GtkApplication*)
{
    // Destroy audio
    Mix_CloseAudio();

    // Destroy UI
    destroy_playlist_ui();
    destroy_playback_ui();
}

static void on_preferences_action(GSimpleAction*, GVariant*, gpointer)
{
    printf("preferences\n");
}

static void on_about_action(GSimpleAction*, GVariant*, gpointer window)
{
    adw_show_about_window(
        window,
        "application-name", "Waveform",
        "developer-name", "Luka Warren",
        "version", "1.0",
        "copyright", "© 2024 Luka Warren",
        "license-type", GTK_LICENSE_GPL_3_0,
        NULL
    );
}

int main(int argc, char** argv)
{
    g_autoptr(AdwApplication) app = NULL;
    app = adw_application_new("com.github.lukawarren.waveform", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}