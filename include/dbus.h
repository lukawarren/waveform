#pragma once
#include "playlist.h"

void init_dbus();
void close_dbus();
void dbus_set_current_playlist_entry(PlaylistEntry* entry);
void dbus_on_playback_toggled();
