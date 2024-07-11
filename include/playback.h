#pragma once
#include <adwaita.h>
#include "audio_stream.h"

typedef struct PlaylistEntry PlaylistEntry;

void init_playback_ui(GtkBuilder* builder);
void destroy_playback_ui();

void update_playback();
void toggle_playback();
void set_new_playback_entry(PlaylistEntry* entry);
void on_audio_stream_advanced(AudioPacket* packet);

// D-Bus
void playback_next();
void playback_previous();
bool playback_is_playing();
