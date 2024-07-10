#include <adwaita.h>
#include <fftw3.h>
#include "playlist.h"
#include "playback.h"
#include "preferences.h"
#include "audio_stream.h"

static void on_close(GtkWidget* app);
static void on_save_playlist(GSimpleAction*, GVariant*, gpointer);
static void on_load_playlist(GSimpleAction*, GVariant*, gpointer);
static void on_add_song(GSimpleAction*, GVariant*, gpointer);
static void on_preferences_action(GSimpleAction*, GVariant*, gpointer);
static void on_about_action(GSimpleAction*, GVariant*, gpointer);

static const GActionEntry app_actions[] =
{
    { "save_playlist",  on_save_playlist,       NULL, NULL, NULL, { 0 } },
    { "load_playlist",  on_load_playlist,       NULL, NULL, NULL, { 0 } },
    { "add_song",       on_add_song,            NULL, NULL, NULL, { 0 } },
	{ "preferences",    on_preferences_action,  NULL, NULL, NULL, { 0 } },
	{ "about",          on_about_action,        NULL, NULL, NULL, { 0 } }
};

// Command line arguments
static char* playlist_argument;
static GOptionEntry option_entries[] =
{
    {
        "playlist",
        'p',
        0,
        G_OPTION_ARG_FILENAME,
        &playlist_argument,
        "Specify playlist to load at startup",
        NULL
    }
};

static void on_activate(GtkApplication* app)
{
    fftwf_make_planner_thread_safe();
    init_preferences();
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
        "app.save_playlist",
        (const char*[]) { "<primary>s", NULL }
    );
    gtk_application_set_accels_for_action(
        app,
        "app.load_playlist",
        (const char*[]) { "<primary>o", NULL }
    );
    gtk_application_set_accels_for_action(
        app,
        "app.add_song",
        (const char*[]) { "<primary>a", NULL }
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

    // Init screens
    init_playlist_ui(builder, GTK_WINDOW(window));
    init_playback_ui(builder);

    // Show window
    gtk_widget_set_visible(GTK_WIDGET(window), true);
    g_object_unref(builder);

    // Use command line arguments, if any
    if (playlist_argument != NULL)
    {
        load_playlist_with_path(playlist_argument);
        free(playlist_argument);
    }

    // Memory clean-up
    g_signal_connect(window, "destroy", G_CALLBACK(on_close), NULL);
}

static void on_close(GtkWidget*)
{
    // Destroy audio
    Mix_CloseAudio();

    // Destroy UI
    destroy_playlist_ui();
    destroy_playback_ui();
    free_preferences();
}

static void on_save_playlist(GSimpleAction*, GVariant*, gpointer)
{
    on_playlist_save();
}

static void on_load_playlist(GSimpleAction*, GVariant*, gpointer)
{
    on_playlist_load();
}

static void on_add_song(GSimpleAction*, GVariant*, gpointer)
{
    on_playlist_entry_add(NULL);
}

static void on_preferences_action(GSimpleAction*, GVariant*, gpointer)
{
    toggle_preferences_window();
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
    // As the program is not properly installed...
    g_setenv("GSETTINGS_SCHEMA_DIR", ".", FALSE);

    // Parse command line arugments
    GError* error = NULL;
    GOptionContext* context = g_option_context_new(" - lightweight music player and visualiser");
    g_option_context_add_main_entries(context, option_entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print("failed to parse arguments: %s\n", error->message);
        exit(1);
    }

    g_autoptr(AdwApplication) app = NULL;
    app = adw_application_new("com.github.lukawarren.waveform", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
