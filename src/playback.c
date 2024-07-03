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

// Actual playback state
GtkMediaStream* stream = NULL;
PlaylistEntry* current_entry = NULL;

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(stack),
        length == 0 ? empty_page : playback_page
    );
}

static void create_media_stream()
{
    // Delete existing stream, if any
    if (stream != NULL)
    {
        gtk_media_stream_pause(stream);
        g_object_unref(stream);
    }

    printf("%s\n", current_entry->path);
    stream = gtk_media_file_new_for_filename("~/Music/Car Music/Rama - 2 AM.mp3");
    gtk_media_stream_set_volume(stream, 1.0);
    gtk_media_stream_play(stream);
}

static void on_backwards(GtkButton*)
{

}

static void on_play(GtkButton*)
{

}

static void on_forwards(GtkButton*)
{

}

void init_playback_ui(GtkBuilder* builder)
{
    stack = GTK_WIDGET(gtk_builder_get_object(builder, "playback_stack"));
    playback_page = GTK_WIDGET(gtk_builder_get_object(builder, "playback_page"));
    empty_page = GTK_WIDGET(gtk_builder_get_object(builder, "playback_empty_page"));

    backwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "backwards_button"));
    play_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));
    forwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "forwards_button"));

    g_signal_connect(backwards_button,  "clicked", G_CALLBACK(on_backwards),    NULL);
    g_signal_connect(play_button,       "clicked", G_CALLBACK(on_play),         NULL);
    g_signal_connect(forwards_button,   "clicked", G_CALLBACK(on_forwards),     NULL);

    playback_on_playlist_changed();

    // Auto-play
    if (g_list_length(playlist) != 0)
        create_media_stream();
}

void playback_on_playlist_changed()
{
    update_stack();

    if (g_list_length(playlist) != 0)
    {
        // If current song has been removed, go back to start
        if (g_list_find(playlist, current_entry) == NULL)
            current_entry = (PlaylistEntry*)playlist->data;

        // Enable buttons
        gtk_widget_set_sensitive(backwards_button, true);
        gtk_widget_set_sensitive(play_button, true);
        gtk_widget_set_sensitive(forwards_button, true);
    }
    else
    {
        // Disable buttons
        gtk_widget_set_sensitive(backwards_button, false);
        gtk_widget_set_sensitive(play_button, false);
        gtk_widget_set_sensitive(forwards_button, false);
    }
}

void destroy_playback_ui()
{
    if (stream != NULL)
        g_object_unref(stream);
}
