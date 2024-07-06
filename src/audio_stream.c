#include "audio_stream.h"
#include "playback.h"
#include "common.h"
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
        g_critical("failed to load %s", entry->path);

    else
        Mix_PlayMusic(stream->music, 0);

    return stream;
}

static void gui_idle_callback(gpointer audio_packet)
{
    AudioPacket* packet = (AudioPacket*)audio_packet;
    on_audio_stream_advanced(packet);
    free(packet);
}

static void on_effect_called(int, void* buffer, int length, void*)
{
    // Enqueue work to GUI thread
    AudioPacket* packet = malloc(sizeof(AudioPacket));
    packet->data = buffer;
    packet->length = length / sizeof(packet->data[0]);
    g_idle_add_once(gui_idle_callback, packet);
}

void toggle_audio_stream(AudioStream* stream)
{
    stream->is_playing = !stream->is_playing;

    if (stream->is_playing)
    {
    #if !(CONTINUE_VISUALISATION_WHEN_PAUSED)
        Mix_RegisterEffect(
            MIX_CHANNEL_POST,
            on_effect_called,
            NULL,
            NULL
        );
    #endif
        Mix_ResumeMusic();
    }

    else
    {
    #if !(CONTINUE_VISUALISATION_WHEN_PAUSED)
        Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
    #endif
        Mix_PauseMusic();
    }
}

void set_audio_stream_progress(AudioStream* stream, double progress)
{
    Mix_SetMusicPosition(progress * Mix_MusicDuration(stream->music));
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
        g_critical("failed to initialise SDL: %s", SDL_GetError());

    Mix_OpenAudio(
        AUDIO_FREQUENCY, // frequency
        AUDIO_F32SYS,    // format
        CHANNELS,        // channels
        PACKET_SIZE      // chunk size
    );

#if CONTINUE_VISUALISATION_WHEN_PAUSED
    Mix_RegisterEffect(
        MIX_CHANNEL_POST,
        on_effect_called,
        NULL,
        NULL
    );
#endif

    float fps = (float)AUDIO_FREQUENCY / (float)PACKET_SIZE;
    printf("running at approx. %.2f FPS\n", fps);
}

void close_audio()
{
    Mix_CloseAudio();
}