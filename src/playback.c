#include <adwaita.h>
#include "playlist.h"
#include "playback.h"

// UI
static GtkWidget* stack;
static GtkWidget* playback_page;
static GtkWidget* empty_page;
static GtkWidget* backwards_button;
static GtkWidget* play_button;
static GtkWidget* forwards_button;
static GtkWidget* playback_slider;
static GtkWidget* playback_bar;

// Actual playback state
GtkMediaStream* stream = NULL;
PlaylistEntry* current_entry = NULL;

static void on_forwards();
void playback_on_playlist_changed();

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(stack),
        length == 0 ? empty_page : playback_page
    );
}

static void on_stream_progress_did_change()
{
    gtk_range_set_value(
        GTK_RANGE(playback_slider),
        (double)gtk_media_stream_get_timestamp(stream) / (double)gtk_media_stream_get_duration(stream)
    );
}

static void destroy_media_stream()
{
    g_object_unref(stream);
    stream = NULL;
}

static void on_stream_did_finish(GtkMediaStream*)
{
    destroy_media_stream();
    on_forwards(NULL);
}

static void create_media_stream()
{
    if (stream != NULL)
        destroy_media_stream();

    // Create stream
    stream = gtk_media_file_new_for_filename(current_entry->path);
    gtk_media_stream_set_volume(stream, 1.0);
    gtk_media_stream_play(stream);

    // Update icon
    gtk_button_set_icon_name(GTK_BUTTON(play_button), "media-playback-pause");

    // Create callback for when timestamp changes
    GClosure* closure = g_cclosure_new(on_stream_progress_did_change, NULL, NULL);
    GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(playback_slider));
    g_object_bind_property_with_closures(
        stream,
        "timestamp",
        adjustment,
        "value",
        0,
        closure,
        NULL
    );

    g_signal_connect(stream, "notify::ended", G_CALLBACK(on_stream_did_finish), NULL);
}

static void on_forwards(GtkButton*)
{
    GList* current_list_entry = g_list_find(playlist, current_entry);

    // Loop back if need be
    if (current_list_entry == NULL || current_list_entry->next == NULL)
        current_entry = (PlaylistEntry*)playlist->data;

    else
        current_entry = (PlaylistEntry*)current_list_entry->next->data;

    update_playback();
}

static void on_backwards(GtkButton*)
{
    GList* current_list_entry = g_list_find(playlist, current_entry);

    // Loop forwards if need be
    if (current_list_entry == NULL || current_list_entry->prev == NULL)
        current_entry = (PlaylistEntry*)playlist->data;

    else
        current_entry = (PlaylistEntry*)current_list_entry->prev->data;

    update_playback();
}

static void on_play(GtkButton*)
{
    if (stream == NULL) return;

    if (gtk_media_stream_get_playing(stream))
    {
        gtk_media_stream_pause(stream);
        gtk_button_set_icon_name(GTK_BUTTON(play_button), "media-playback-start");
    }
    else
    {
        gtk_media_stream_play(stream);
        gtk_button_set_icon_name(GTK_BUTTON(play_button), "media-playback-pause");
    }

    set_current_playlist_entry(current_entry, gtk_media_stream_get_playing(stream));
}

static void on_slider_moved(GtkRange*, GtkScrollType*, gdouble value, gpointer)
{
    if (stream == NULL)
        return;

    gtk_media_stream_set_loop(stream, true);

    gtk_media_stream_seek(stream,
        (gint64)(value * (double)gtk_media_stream_get_duration(stream))
    );

    gtk_media_stream_set_loop(stream, false);
}

void init_playback_ui(GtkBuilder* builder)
{
    stack = GTK_WIDGET(gtk_builder_get_object(builder, "playback_stack"));
    playback_page = GTK_WIDGET(gtk_builder_get_object(builder, "playback_page"));
    empty_page = GTK_WIDGET(gtk_builder_get_object(builder, "playback_empty_page"));

    backwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "backwards_button"));
    play_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));
    forwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "forwards_button"));

    playback_slider = GTK_WIDGET(gtk_builder_get_object(builder, "playback_slider"));
    playback_bar = GTK_WIDGET(gtk_builder_get_object(builder, "playback_bar"));

    g_signal_connect(backwards_button,  "clicked",      G_CALLBACK(on_backwards),    NULL);
    g_signal_connect(play_button,       "clicked",      G_CALLBACK(on_play),         NULL);
    g_signal_connect(forwards_button,   "clicked",      G_CALLBACK(on_forwards),     NULL);
    g_signal_connect(playback_slider,   "change-value", G_CALLBACK(on_slider_moved), NULL);

    update_playback();
}

void update_playback()
{
    update_stack();

    if (g_list_length(playlist) != 0)
    {
        // If current song has been removed, go back to start
        if (g_list_find(playlist, current_entry) == NULL)
            current_entry = (PlaylistEntry*)playlist->data;

        // Enable buttons
        gtk_widget_set_sensitive(playback_bar, true);

        // Auto-play
        create_media_stream();
    }
    else
    {
        // Disable buttons
        gtk_widget_set_sensitive(playback_bar, false);

        // Destroy stream
        if (stream != NULL)
            destroy_media_stream();
    }

    set_current_playlist_entry(
        current_entry,
        stream == NULL ? false : gtk_media_stream_get_playing(stream)
    );
}

void toggle_playback()
{
    on_play(NULL);
}

void destroy_playback_ui()
{
    if (stream != NULL)
        destroy_media_stream();
}
