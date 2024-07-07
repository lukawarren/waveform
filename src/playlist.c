#include <adwaita.h>
#include <SDL2/SDL_mixer.h>
#include "playlist.h"
#include "playback.h"
#include "common.h"

GList* playlist = NULL;

static GtkWidget* playlist_list;
static GtkWidget* playlist_stack;
static GtkWidget* playlist_page;
static GtkWidget* empty_page;
static GtkWindow* window;

static GtkWidget* create_ui_playlist_entry(PlaylistEntry* playlist_entry);

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
    gtk_list_box_remove(GTK_LIST_BOX(playlist_list), widget);

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

static GdkContentProvider* on_drag_prepare(GtkDragSource*, double, double, GtkWidget* self)
{
    return gdk_content_provider_new_typed(adw_action_row_get_type(), self);
}

static void on_drag_begin(GtkDragSource*, GdkDrag* drag, GtkWidget* widget)
{
    // Get playlist entry
    PlaylistEntry* entry = (PlaylistEntry*)g_object_get_data(G_OBJECT(widget), "playlist_entry");

    // Work out size for new dummy widget
    int width = gtk_widget_get_width(widget);
    int height = gtk_widget_get_height(widget);

    // Create new dummy widget
    GtkWidget* dummy_widget = create_ui_playlist_entry(entry);
    gtk_widget_add_css_class(dummy_widget, "drag-entry");
    gtk_widget_set_size_request(dummy_widget, width, height);

    GtkWidget* icon = gtk_drag_icon_get_for_drag(drag);
    gtk_drag_icon_set_child(GTK_DRAG_ICON(icon), dummy_widget);
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

    // Same with playlist entry for drag and drop
    g_object_set_data(G_OBJECT(entry), "playlist_entry", playlist_entry);

    // Drag and drop source
    GtkDragSource* drag_source = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag_source, GDK_ACTION_MOVE);
    gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(drag_source));
    g_signal_connect(drag_source, "prepare", G_CALLBACK(on_drag_prepare), entry);
    g_signal_connect(drag_source, "drag-begin", G_CALLBACK(on_drag_begin), entry);

    // Drag and drop dest
    GtkDropTarget* target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_MOVE);
    gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(target));

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
    gtk_list_box_append(GTK_LIST_BOX(playlist_list), widget);

    // Create link between playlist entry and UI for later logic
    g_object_set_data(G_OBJECT(widget), "playlist_entry", entry);

    // Bind list entry to button for event callback
    g_object_set_data(G_OBJECT(widget), "entry", entry);
}

static bool add_file_to_playlist(GFile* file)
{
    // Create SDL music object
    char* file_path = g_file_get_path(file);
    Mix_Music* music = Mix_LoadMUS(file_path);
    if (music == NULL)
    {
        g_critical("failed to load %s", file_path);
        g_free(file_path);
        return false;
    }

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
    return true;
}

static void on_playlist_add_dialog_ready(GObject* dialog, GAsyncResult* result, gpointer)
{
    GListModel* list = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(dialog), result, NULL);
    if (list == NULL)
    {
        g_object_unref(dialog);
        return;
    }

    bool one_or_more_failures = false;
    for (guint i = 0; i < g_list_model_get_n_items(list); ++i)
    {
        GFile* file = g_list_model_get_item(list, i);
        if (!add_file_to_playlist(file))
            one_or_more_failures = true;
        g_object_unref(file);
    }

    if (one_or_more_failures)
    {
        GtkAlertDialog* alert = gtk_alert_dialog_new("Failed To Load Selection");
        gtk_alert_dialog_set_detail(alert, "One or more files failed to load");
        gtk_alert_dialog_show(alert, window);
    }

    g_object_unref(list);
    g_object_unref(dialog);

    // Update rest of program
    update_stack();
    update_playback();
}

static void set_dialog_directory(GtkFileDialog* dialog)
{
    const gchar* music_location = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC);
    GFile* music_folder = g_file_new_for_path(music_location);
    gtk_file_dialog_set_initial_folder(dialog, music_folder);
    g_object_unref(music_folder);
}

void on_playlist_entry_add(GtkButton*)
{
    GtkFileDialog* dialog = gtk_file_dialog_new();
    set_dialog_directory(dialog);

    // Set filter to audio files only
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Audio Files");
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
    playlist_list   = GET_WIDGET("playlist_list");
    playlist_stack  = GET_WIDGET("playlist_stack");
    playlist_page   = GET_WIDGET("playlist_page");
    empty_page      = GET_WIDGET("playlist_empty_page");
    window = _window;

    // Add button
    GtkWidget* playlist_add_button = GET_WIDGET("playlist_add_button");
    g_signal_connect(playlist_add_button, "clicked", G_CALLBACK(on_playlist_entry_add), NULL);
}

void destroy_playlist_ui()
{
    g_list_free_full(playlist, free_playlist_entry);
}

void set_current_playlist_entry(PlaylistEntry* entry, bool is_playing)
{
    GtkWidget* row = gtk_widget_get_first_child(playlist_list);
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

static void on_save_dialog_done(GObject* self, GAsyncResult* result, gpointer)
{
    // Get path
    GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(self), result, NULL);
    if (file == NULL) return;

    if (g_file_query_exists(file, NULL))
        g_file_delete(file, NULL, NULL);

    GFileOutputStream* stream = g_file_create(
        file,
        G_FILE_CREATE_NONE,
        NULL,
        NULL
    );

    if (stream != NULL)
    {
        GList* current = playlist;
        while (current != NULL)
        {
            PlaylistEntry* entry = (PlaylistEntry*)current->data;
            g_output_stream_printf(
                G_OUTPUT_STREAM(stream),
                NULL,
                NULL,
                NULL,
                "%s\n", entry->path
            );
            current = current->next;
        }

        g_output_stream_close(G_OUTPUT_STREAM(stream), NULL, NULL);
        g_object_unref(stream);
    }
    else
        g_critical("failed to create output stream");

    g_object_unref(file);
}

static bool load_playlist_from_file(GFile* file)
{
    GFileInputStream* stream = g_file_read(file, NULL, NULL);
    if (stream != NULL)
    {
        // Delete old playlist
        g_list_free_full(playlist, free_playlist_entry);
        playlist = NULL;
        gtk_list_box_remove_all(GTK_LIST_BOX(playlist_list));

        // Read each line
        GDataInputStream* data_stream = g_data_input_stream_new(G_INPUT_STREAM(stream));
        bool one_or_more_failures = false;
        while (true)
        {
            gsize length;
            char* line = g_data_input_stream_read_line(data_stream, &length, NULL, NULL);

            if (line == NULL || length == 0)
                break;

            // file will never be NULL
            GFile* file = g_file_new_for_path(line);
            if (!add_file_to_playlist(file))
                one_or_more_failures = true;
            g_object_unref(file);
            g_free(line);
        }

        if (one_or_more_failures)
        {
            GtkAlertDialog* alert = gtk_alert_dialog_new("Failed To Load Playlist");
            gtk_alert_dialog_set_detail(alert, "One or more files failed to load");
            gtk_alert_dialog_show(alert, window);
        }

        // Update UI
        update_stack();
        update_playback();

        g_object_unref(data_stream);
        g_object_unref(stream);
        return true;
    }

    return false;
}

static void on_load_dialog_done(GObject* self, GAsyncResult* result, gpointer)
{
    GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(self), result, NULL);
    if (file != NULL)
    {
        load_playlist_from_file(file);
        g_object_unref(file);
    }
}

void on_playlist_save()
{
    if (g_list_length(playlist) == 0)
    {
        GtkAlertDialog* alert = gtk_alert_dialog_new("Unable To Save Playlist");
        gtk_alert_dialog_set_detail(alert, "Playlist is currently empty");
        gtk_alert_dialog_show(alert, window);
        return;
    }

    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_initial_name(dialog, "playlist.waveform");
    set_dialog_directory(dialog);
    gtk_file_dialog_save(dialog, window, NULL, on_save_dialog_done, NULL);
}

void on_playlist_load()
{
    GtkFileDialog* dialog = gtk_file_dialog_new();
    set_dialog_directory(dialog);
    gtk_file_dialog_set_title(dialog, "Select a Playlist");

    // Set filter to playlist files
    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Playlist Files");
    gtk_file_filter_add_suffix(filter, "waveform");
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filter);
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, window, NULL, on_load_dialog_done, NULL);
}

void load_playlist_with_path(const char* path)
{
    GFile* file = g_file_new_for_path(path);
    if (!load_playlist_from_file(file))
        g_critical("failed to load playlist %s", path);
    g_object_unref(file);
}
