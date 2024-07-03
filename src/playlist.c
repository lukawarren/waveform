#include <adwaita.h>

static GtkWidget* playlist_list;
static GtkWidget* playlist_stack;
static GtkWidget* playlist_page;
static GtkWidget* empty_page;

static void on_playlist_entry_removed(GtkButton* button)
{
    // Find row in list
    GtkWidget* entry = GTK_WIDGET(button);
    for (int i = 0; i < 3; ++i)
        entry = gtk_widget_get_parent(entry);

    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(playlist_list), entry);
}

static void on_playlist_entry_add(GtkButton*)
{
    adw_view_stack_set_visible_child(ADW_VIEW_STACK(playlist_stack), playlist_page);
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

static void add_ui_playlist_entry(const char* title, const char* subtitle)
{
    GtkWidget* entry = create_ui_playlist_entry(title, subtitle);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(playlist_list), entry);
}

void init_playlist_ui(GtkBuilder* builder)
{
    // Get objects
    playlist_list = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_list"));
    playlist_stack = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_stack"));
    playlist_page = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_page"));
    empty_page = GTK_WIDGET(gtk_builder_get_object(builder, "empty_page"));

    // Add button
    GtkWidget* playlist_add_button = GTK_WIDGET(gtk_builder_get_object(builder, "playlist_add_button"));
    g_signal_connect(playlist_add_button, "clicked", G_CALLBACK(on_playlist_entry_add), NULL);

    add_ui_playlist_entry("JAY-Z - 4:44", "4:47");
    add_ui_playlist_entry("B Young - 079ME", "3:31");
    add_ui_playlist_entry("21 Savage - asmr", "2:53");
}