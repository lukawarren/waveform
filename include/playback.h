#pragma once
#include <adwaita.h>

void init_playback_ui(GtkBuilder* builder);
void destroy_playback_ui();
void playback_on_playlist_changed();
