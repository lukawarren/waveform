#include <adwaita.h>
#include "playlist.h"
#include "playback.h"

// UI state
static GtkWidget* stack;
static GtkWidget* playback_page;
static GtkWidget* empty_page;

// Actual playback state
size_t current_song = 0;

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(stack),
        length == 0 ? empty_page : playback_page
    );
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

    GtkWidget* backwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "backwards_button"));
    GtkWidget* play_button = GTK_WIDGET(gtk_builder_get_object(builder, "play_button"));
    GtkWidget* forwards_button = GTK_WIDGET(gtk_builder_get_object(builder, "forwards_button"));

    g_signal_connect(backwards_button,  "clicked", G_CALLBACK(on_backwards),    NULL);
    g_signal_connect(play_button,       "clicked", G_CALLBACK(on_play),         NULL);
    g_signal_connect(forwards_button,   "clicked", G_CALLBACK(on_forwards),     NULL);
}

void playback_on_playlist_changed()
{
    update_stack();

    if (g_list_length(playlist) != 0)
        adw_status_page_set_title(ADW_STATUS_PAGE(playback_page), ((PlaylistEntry*)playlist->data)->name);
}
