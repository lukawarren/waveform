#include "audio_stream.h"
#include <stdlib.h>
#include <stdio.h>
#include <adwaita.h>
#include <SDL2/SDL.h>

AudioStream* create_audio_stream(PlaylistEntry* entry)
{
    AudioStream* stream = malloc(sizeof(AudioStream));
    stream->is_playing = false;
    stream->playlist_entry = entry;
    stream->music = Mix_LoadMUS(entry->path);

    if (stream->music == NULL)
        g_critical("failed to load %s\n", entry->path);

    else
        Mix_PlayMusic(stream->music, 0);

    return stream;
}

void toggle_audio_stream(AudioStream* stream)
{
    stream->is_playing = !stream->is_playing;

    if (stream->is_playing)
        Mix_ResumeMusic();

    else
        Mix_PauseMusic();
}

void free_audio_stream(AudioStream* stream)
{
    if (stream->music != NULL)
        Mix_FreeMusic(stream->music);

    free(stream);
}

void init_audio()
{
    // Init SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
        g_critical("failed to initialise SDL: %s\n", SDL_GetError());

    Mix_OpenAudio(
        48000,          // frequency
        AUDIO_F32SYS,   // format
        2,              // channels
        1024            // chunk size
    );
}

void close_audio()
{
    Mix_CloseAudio();
}