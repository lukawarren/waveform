#include <adwaita.h>
#include "playlist.h"
#include "playback.h"
#include "audio_stream.h"

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
PlaylistEntry* current_entry = NULL;
AudioStream* audio_stream = NULL;

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(stack),
        length == 0 ? empty_page : playback_page
    );
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
    toggle_audio_stream(audio_stream);
    set_current_playlist_entry(current_entry, audio_stream->is_playing);

    if (audio_stream->is_playing)
        gtk_button_set_icon_name(GTK_BUTTON(play_button), "media-playback-pause");
    else
        gtk_button_set_icon_name(GTK_BUTTON(play_button), "media-playback-start");
}

static void on_slider_moved(GtkRange*, GtkScrollType*, gdouble value, gpointer)
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

    playback_slider = GTK_WIDGET(gtk_builder_get_object(builder, "playback_slider"));
    playback_bar = GTK_WIDGET(gtk_builder_get_object(builder, "playback_bar"));

    g_signal_connect(backwards_button,  "clicked",      G_CALLBACK(on_backwards),    NULL);
    g_signal_connect(play_button,       "clicked",      G_CALLBACK(on_play),         NULL);
    g_signal_connect(forwards_button,   "clicked",      G_CALLBACK(on_forwards),     NULL);
    g_signal_connect(playback_slider,   "change-value", G_CALLBACK(on_slider_moved), NULL);

    update_playback();
}

static void destroy_audio_stream()
{
    if (audio_stream != NULL)
        free_audio_stream(audio_stream);
    audio_stream = NULL;
}

void update_playback()
{
    if (g_list_length(playlist) != 0)
    {
        // If current song has been removed, go back to start
        if (g_list_find(playlist, current_entry) == NULL)
            current_entry = (PlaylistEntry*)playlist->data;

        // Enable buttons
        gtk_widget_set_sensitive(playback_bar, true);

        // Create new audio stream if song changed
        if (audio_stream == NULL || current_entry != audio_stream->playlist_entry)
        {
            destroy_audio_stream();
            audio_stream = create_audio_stream(current_entry);
            on_play(NULL);
        }
    }
    else
    {
        // Disable buttons
        gtk_widget_set_sensitive(playback_bar, false);
        destroy_audio_stream();
    }

    update_stack();
}

void toggle_playback()
{
    on_play(NULL);
}

void destroy_playback_ui()
{
    destroy_audio_stream();
}
