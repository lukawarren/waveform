#pragma once
#include <adwaita.h>

typedef struct PlaylistEntry
{
    const gchar* name;
    const gchar* duration;
    const gchar* path;
} PlaylistEntry;
extern GList* playlist;

void init_playlist_ui(GtkBuilder* builder, GtkWindow* window);
void destroy_playlist_ui();
void set_current_playlist_entry(PlaylistEntry* entry, bool is_playing);
