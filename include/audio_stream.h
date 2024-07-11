#pragma once
#include <stdbool.h>
#include "playlist.h"
#include <SDL_mixer.h>

typedef struct AudioStream
{
    bool is_playing;
    Mix_Music* music;
    PlaylistEntry* playlist_entry;
    double fade_pos;
} AudioStream;

typedef struct AudioPacket
{
    float* data;
    int length;
} AudioPacket;

AudioStream* create_audio_stream(PlaylistEntry* entry);
void toggle_audio_stream(AudioStream* stream);
void set_audio_stream_progress(AudioStream* stream, double progress);
void free_audio_stream(AudioStream* stream);

void init_audio();
void mute_audio();
void unmute_audio();
void set_audio_speed(float speed);
void close_audio();
