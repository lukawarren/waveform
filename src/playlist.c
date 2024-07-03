#include <adwaita.h>

typedef struct PlaylistEntry
{
    const char* name;
    const char* duration;
    const char* path;
} PlaylistEntry;
static GList* playlist = NULL;

static GtkWidget* playlist_list;
static GtkWidget* playlist_stack;
static GtkWidget* playlist_page;
static GtkWidget* empty_page;
static GtkWindow* window;

static void handle_stack()
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
    handle_stack();
}

static GtkWidget* create_ui_playlist_entry(const char* title, const char* subtitle)
{
    // Row
    GtkWidget* entry = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), title);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(entry), subtitle);

    // Button
    GtkWidget* button = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(button), "edit-delete-symbolic");
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(button, "flat");
    g_signal_connect(button, "clicked", G_CALLBACK(on_playlist_entry_removed), NULL);
    adw_action_row_add_suffix(ADW_ACTION_ROW(entry), button);

    return entry;
}

static void add_playlist_entry(const char* name, const char* duration, const char* path)
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

void on_playlist_add_dialog_ready(GObject* dialog, GAsyncResult* result, gpointer)
{
    GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(dialog), result, NULL);
    if (file == NULL) return;

    // Allocate strings
    char* file_name = g_file_get_basename(file);
    char* file_duration = malloc(sizeof("1:23"));
    strcpy(file_duration, "1:23");
    gchar* file_path = g_file_get_path(file);

    // Add to program
    add_playlist_entry(file_name, file_duration, file_path);
    handle_stack();

    g_object_unref(file);
    g_object_unref(dialog);
}

static void on_playlist_entry_add(GtkButton*)
{
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_open(
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
    empty_page = GTK_WIDGET(gtk_builder_get_object(builder, "empty_page"));
    window = _window;

    // Add button
    GtkWidget* playlist_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_add_button"));
    g_signal_connect(playlist_add_button, "clicked", G_CALLBACK(on_playlist_entry_add), NULL);
}

void destroy_playlist_ui()
{
    g_list_free_full(playlist, free_playlist_entry);
}
