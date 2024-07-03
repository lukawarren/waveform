#include <adwaita.h>
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
    g_free((void*)((PlaylistEntry*)entry)->duration);
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
    playback_on_playlist_changed();
}

static GtkWidget* create_ui_playlist_entry(const gchar* title, const gchar* subtitle)
{
    // Row
    GtkWidget* entry = adw_action_row_new();

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), title);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(entry), subtitle);

    // Remove button
    GtkWidget* button = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(button), "edit-delete-symbolic");
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(button, "flat");
    g_signal_connect(button, "clicked", G_CALLBACK(on_playlist_entry_removed), NULL);
    adw_action_row_add_suffix(ADW_ACTION_ROW(entry), button);

    return entry;
}

static void add_playlist_entry(const char* name, const gchar* duration, const gchar* path)
{
    // Add to playlist
    PlaylistEntry* entry = malloc(sizeof(PlaylistEntry));
    entry->name = name;
    entry->duration = duration;
    entry->path = path;
    playlist = g_list_append(playlist, entry);

    // Add to UI
    GtkWidget* widget = create_ui_playlist_entry(name, duration);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(playlist_list), widget);

    // Bind list entry to button for event callback
    g_object_set_data(G_OBJECT(widget), "entry", entry);
}

static void add_file_to_playlist(GFile* file)
{
    // Allocate strings
    char* file_name = g_file_get_basename(file);
    char* file_duration = malloc(sizeof("1:23"));
    strcpy(file_duration, "1:23");
    gchar* file_path = g_file_get_path(file);

    // Sanitise for escape sequences
    gchar* file_name_s = g_markup_escape_text(file_name, -1);
    gchar* file_path_s = g_markup_escape_text(file_path, -1);
    free(file_name);
    free(file_path);

    // Add to program
    add_playlist_entry(file_name_s, file_duration, file_path_s);
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
    playback_on_playlist_changed();
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
    add_file_to_playlist(g_file_new_for_path("~/Music/Car Music/Rama - 2 AM.mp3"));
    update_stack();
    playback_on_playlist_changed();
}

void destroy_playlist_ui()
{
    g_list_free_full(playlist, free_playlist_entry);
}
