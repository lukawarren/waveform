#include <adwaita.h>
#include "playlist.h"
#include "playback.h"
#include "visualiser.h"
#include "common.h"

// UI
static GtkWidget* stack;
static GtkWidget* playback_page;
static GtkWidget* empty_page;
static GtkWidget* backwards_button;
static GtkWidget* play_button;
static GtkWidget* forwards_button;
static GtkWidget* playback_slider;
static GtkWidget* playback_bar;
static GtkWidget* drawing_area;
static GtkWidget* mute_button;
static GtkWidget* shuffle_button;

// Actual playback state
static PlaylistEntry* current_entry = NULL;
static AudioStream* audio_stream = NULL;
static bool shuffle = false;

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(stack),
        length == 0 ? empty_page : playback_page
    );
}

static void select_random_song()
{
    GList* current_list_entry = g_list_find(playlist, current_entry);
    guint length = g_list_length(playlist);

    while (true)
    {
        GList* entry = g_list_nth(playlist, rand() % length);
        if (entry != current_list_entry)
        {
            current_entry = (PlaylistEntry*)entry->data;
            break;
        }
    }
}

static void on_forwards(GtkButton*)
{
    if (g_list_length(playlist) == 1)
    {
        set_audio_stream_progress(audio_stream, 0.0f);
        return;
    }

    if (!shuffle)
    {
        GList* current_list_entry = g_list_find(playlist, current_entry);

        // Loop back if need be
        if (current_list_entry->next == NULL)
            current_entry = (PlaylistEntry*)playlist->data;

        else
            current_entry = (PlaylistEntry*)current_list_entry->next->data;
    }
    else
        select_random_song();

    update_playback();
}

static void on_backwards(GtkButton*)
{
    if (g_list_length(playlist) == 1)
    {
        set_audio_stream_progress(audio_stream, 0.0f);
        return;
    }

    if (!shuffle)
    {
        GList* current_list_entry = g_list_find(playlist, current_entry);

        // Loop back if  need be
        if (current_list_entry->prev == NULL)
            current_entry = (PlaylistEntry*)g_list_last(playlist)->data;

        else
            current_entry = (PlaylistEntry*)current_list_entry->prev->data;
    }
    else
        select_random_song();

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
    set_audio_stream_progress(audio_stream, value);
}

static void on_mute(GtkToggleButton*)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mute_button)))
        mute_audio();
    else
        unmute_audio();
}

static void on_shuffle(GtkToggleButton*)
{
    shuffle = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(shuffle_button));
}

void init_playback_ui(GtkBuilder* builder)
{
    stack               = GET_WIDGET("playback_stack");
    playback_page       = GET_WIDGET("playback_page");
    empty_page          = GET_WIDGET("playback_empty_page");
    drawing_area        = GET_WIDGET("drawing_area");
    backwards_button    = GET_WIDGET("backwards_button");
    play_button         = GET_WIDGET("play_button");
    forwards_button     = GET_WIDGET("forwards_button");
    playback_slider     = GET_WIDGET("playback_slider");
    playback_bar        = GET_WIDGET("playback_bar");
    mute_button         = GET_WIDGET("mute_button");
    shuffle_button      = GET_WIDGET("shuffle_button");

    g_signal_connect(backwards_button,  "clicked",      G_CALLBACK(on_backwards),    NULL);
    g_signal_connect(play_button,       "clicked",      G_CALLBACK(on_play),         NULL);
    g_signal_connect(forwards_button,   "clicked",      G_CALLBACK(on_forwards),     NULL);
    g_signal_connect(playback_slider,   "change-value", G_CALLBACK(on_slider_moved), NULL);
    g_signal_connect(mute_button,       "clicked",      G_CALLBACK(on_mute),         NULL);
    g_signal_connect(shuffle_button,    "clicked",      G_CALLBACK(on_shuffle),      NULL);

    visualiser_init(drawing_area);

    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(drawing_area),
        visualiser_draw_function,
        NULL,
        NULL
    );

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

void set_new_playback_entry(PlaylistEntry* entry)
{
    current_entry = entry;
    update_playback();
}

void destroy_playback_ui()
{
    destroy_audio_stream();
    visualiser_free_data();
}

void on_audio_stream_advanced(AudioPacket* packet)
{
    // Since we enqueued the work, the audio stream may have been removed
    if (audio_stream == NULL)
        return;

    // Send data to visualiser
    visualiser_set_data(packet);

    // Update UI
    double progress = Mix_GetMusicPosition(audio_stream->music) /
        Mix_MusicDuration(audio_stream->music);
    gtk_range_set_value(GTK_RANGE(playback_slider), progress);
    gtk_widget_queue_draw(drawing_area);

    // Go to next song when this one finishes
    if (progress >= 1.0)
        on_forwards(NULL);
}
