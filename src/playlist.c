#include <adwaita.h>
#include <SDL2/SDL_mixer.h>
#include "playlist.h"
#include "playback.h"

GList* playlist = NULL;

static GtkWidget* playlist_list;
static GtkWidget* playlist_stack;
static GtkWidget* playlist_page;
static GtkWidget* empty_page;
static GtkWindow* window;

static void update_stack()
{
    gint length = g_list_length(playlist);
    adw_view_stack_set_visible_child(
        ADW_VIEW_STACK(playlist_stack),
        length == 0 ? empty_page : playlist_page
    );
}

static void free_playlist_entry(void* entry)
{
    g_free((void*)((PlaylistEntry*)entry)->name);
    g_free((void*)((PlaylistEntry*)entry)->artist);
    g_free((void*)((PlaylistEntry*)entry)->path);
    free(entry);
}

static void on_playlist_entry_removed(GtkButton* button)
{
    // Find row in list
    GtkWidget* widget = GTK_WIDGET(button);
    for (int i = 0; i < 3; ++i)
        widget = gtk_widget_get_parent(widget);

    // Remove from playlist
    PlaylistEntry* entry = g_object_get_data(G_OBJECT(widget), "entry");
    free_playlist_entry(entry);
    playlist = g_list_remove(playlist, entry);

    // Remove from UI
    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(playlist_list), widget);

    // Update rest of state
    update_stack();
    update_playback();
}

static void on_playlist_entry_clicked(GtkGestureClick*, gint, gdouble, gdouble, gpointer data)
{
    PlaylistEntry* entry = (PlaylistEntry*)data;
    set_new_playback_entry(entry);
}

static void on_playlist_entry_playback_toggled(GtkButton*)
{
    toggle_playback();
}

static GtkWidget* create_ui_playlist_entry(PlaylistEntry* playlist_entry)
{
    // Row
    GtkWidget* entry = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), playlist_entry->name);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(entry), playlist_entry->artist);

    // Make row clickable
    GtkGesture* gesture = gtk_gesture_click_new();
    g_signal_connect(gesture, "released", G_CALLBACK(on_playlist_entry_clicked), playlist_entry);
    gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(gesture));

    // Pause button
    GtkWidget* pause_button = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(pause_button), "media-playback-pause");
    gtk_widget_set_valign(pause_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(pause_button, "flat");
    gtk_widget_set_visible(pause_button, false);
    g_signal_connect(pause_button, "clicked", G_CALLBACK(on_playlist_entry_playback_toggled), NULL);
    adw_action_row_add_suffix(ADW_ACTION_ROW(entry), pause_button);

    // Remove button
    GtkWidget* remove_button = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(remove_button), "edit-delete-symbolic");
    gtk_widget_set_valign(remove_button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(remove_button, "flat");
    g_signal_connect(remove_button, "clicked", G_CALLBACK(on_playlist_entry_removed), NULL);
    adw_action_row_add_suffix(ADW_ACTION_ROW(entry), remove_button);

    // Easy way to find pause button once row found
    g_object_set_data(G_OBJECT(entry), "pause_button", pause_button);

    return entry;
}

static void add_playlist_entry(const char* name, const gchar* artist, const gchar* path)
{
    // Add to playlist
    PlaylistEntry* entry = malloc(sizeof(PlaylistEntry));
    entry->name = name;
    entry->artist = artist;
    entry->path = path;
    playlist = g_list_append(playlist, entry);

    // Add to UI
    GtkWidget* widget = create_ui_playlist_entry(entry);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(playlist_list), widget);

    // Create link between playlist entry and UI for later logic
    g_object_set_data(G_OBJECT(widget), "playlist_entry", entry);

    // Bind list entry to button for event callback
    g_object_set_data(G_OBJECT(widget), "entry", entry);
}

static void add_file_to_playlist(GFile* file)
{
    // Create SDL music object
    char* file_path = g_file_get_path(file);
    Mix_Music* music = Mix_LoadMUS(file_path);
    if (music == NULL)
        g_critical("failed to load %s\n", file_path);

    // Extract info
    const char* music_title = Mix_GetMusicTitle(music);
    const char* music_artist = Mix_GetMusicArtistTag(music);

    // Allocate title
    char* name;
    if (strcmp(music_title, "") != 0)
    {
        name = malloc(strlen(music_title) + 1);
        strcpy(name, music_title);
    }
    else name = g_file_get_basename(file);

    // Allocate artist
    if (strcmp(music_artist, "") == 0)
        music_artist = "Unknown artist";
    char* artist = malloc(strlen(music_artist) + 1);
    strcpy(artist, music_artist);

    // Sanitise for escape sequences
    gchar* name_s = g_markup_escape_text(name, -1);
    gchar* artist_s = g_markup_escape_text(artist, -1);
    free(name);
    free(artist);
    Mix_FreeMusic(music);

    // Add to program
    add_playlist_entry(name_s, artist_s, file_path);
}

static void on_playlist_add_dialog_ready(GObject* dialog, GAsyncResult* result, gpointer)
{
    GListModel* list = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(dialog), result, NULL);
    if (list == NULL)
    {
        g_object_unref(dialog);
        return;
    }

    for (guint i = 0; i < g_list_model_get_n_items(list); ++i)
    {
        GFile* file = g_list_model_get_item(list, i);
        add_file_to_playlist(file);
    }

    g_object_unref(list);
    g_object_unref(dialog);

    // Update rest of program
    update_stack();
    update_playback();
}

static void on_playlist_entry_add(GtkButton*)
{
    GtkFileDialog* dialog = gtk_file_dialog_new();

    // Pick music folder
    const gchar* music_location = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
    GFile* music_folder = g_file_new_for_path(music_location);
    gtk_file_dialog_set_initial_folder(dialog, music_folder);
    g_object_unref(music_folder);

    // Set filter to audio files only
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio files");
    gtk_file_filter_add_mime_type(filter, "audio/*");
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filter);
    g_object_unref(filters);

    gtk_file_dialog_open_multiple(
        dialog,
        window,
        NULL,
        on_playlist_add_dialog_ready,
        NULL
    );
}

void init_playlist_ui(GtkBuilder* builder, GtkWindow* _window)
{
    // Get objects
    playlist_list = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_list"));
    playlist_stack = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_stack"));
    playlist_page = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_page"));
    empty_page = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_empty_page"));
    window = _window;

    // Add button
    GtkWidget* playlist_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_add_button"));
    g_signal_connect(playlist_add_button, "clicked", G_CALLBACK(on_playlist_entry_add), NULL);

    // Dummy
    add_file_to_playlist(g_file_new_for_path("/home/luka/Music/Car Music/Rama - 2 AM.mp3"));
    add_file_to_playlist(g_file_new_for_path("/home/luka/Music/Car Music/SL - Tropical.mp3"));
    update_stack();
}

void destroy_playlist_ui()
{
    g_list_free_full(playlist, free_playlist_entry);
}

void set_current_playlist_entry(PlaylistEntry* entry, bool is_playing)
{
    // Find container for rows
    GtkWidget* root = gtk_widget_get_first_child(playlist_list);
    root = gtk_widget_get_first_child(root);
    root = gtk_widget_get_next_sibling(root);
    root = gtk_widget_get_first_child(root);

    GtkWidget* row = gtk_widget_get_first_child(root);
    while (row != NULL)
    {
        if (strcmp(gtk_widget_get_name(row), "AdwActionRow") == 0)
        {
            // Row found; ammend
            GtkWidget* pause_button = (GtkWidget*)g_object_get_data(G_OBJECT(row), "pause_button");
            PlaylistEntry* playlist_entry = (PlaylistEntry*) g_object_get_data(G_OBJECT(row), "playlist_entry");
            gtk_widget_set_visible(pause_button, playlist_entry == entry);
            gtk_button_set_icon_name(
                GTK_BUTTON(pause_button),
                is_playing ? "media-playback-pause" : "media-playback-start"
            );
        }

        row = gtk_widget_get_next_sibling(row);
    }
}
