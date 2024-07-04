#pragma once
#include <adwaita.h>

void init_playback_ui(GtkBuilder* builder);
void destroy_playback_ui();
void update_playback();
void toggle_playback();
void on_audio_stream_advanced();
