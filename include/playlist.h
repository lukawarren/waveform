#pragma once
#include <adwaita.h>

typedef struct PlaylistEntry
{
    const gchar* name;
    const gchar* artist;
    const gchar* path;
} PlaylistEntry;
extern GList* playlist;

void init_playlist_ui(GtkBuilder* builder, GtkWindow* window);
void destroy_playlist_ui();

void set_current_playlist_entry(PlaylistEntry* entry, bool is_playing);
void on_playlist_entry_add(GtkButton*);
void on_playlist_save();
void on_playlist_load();

void add_playlist_with_path(const char* path);
void add_file_with_path(const char* path);
