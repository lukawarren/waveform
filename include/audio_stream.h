#pragma once
#include <stdbool.h>
#include <SDL2/SDL_mixer.h>
#include "playlist.h"

typedef struct AudioStream
{
    bool is_playing;
    Mix_Music* music;
    PlaylistEntry* playlist_entry;
} AudioStream;

AudioStream* create_audio_stream(PlaylistEntry* entry);
void toggle_audio_stream(AudioStream* stream);
void set_audio_stream_progress(AudioStream* stream, double progress);
void free_audio_stream(AudioStream* stream);

void init_audio();
void close_audio();