#include "audio_stream.h"
#include "playback.h"
#include "common.h"
#include "preferences.h"
#include <stdlib.h>
#include <stdio.h>
#include <adwaita.h>
#include <SDL2/SDL.h>

static bool muted = false;
static float playback_speed;

AudioStream* create_audio_stream(PlaylistEntry* entry)
{
#if !(CONTINUE_VISUALISATION_WHEN_PAUSED)
    // The current effect may still be present
    Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
#endif

    // Create stream and load music
    AudioStream* stream = malloc(sizeof(AudioStream));
    stream->is_playing = false;
    stream->playlist_entry = entry;
    stream->music = Mix_LoadMUS(entry->path);
    stream->fade_pos = 0.0f;

    if (stream->music == NULL)
        g_critical("failed to load %s", entry->path);

    return stream;
}

static void gui_idle_callback(gpointer audio_packet)
{
    AudioPacket* packet = (AudioPacket*)audio_packet;
    on_audio_stream_advanced(packet);
    free(packet->data);
    free(packet);
}

static void on_effect_called(int, void* buffer, int length, void*)
{
    // Enqueue work to GUI thread
    AudioPacket* packet = malloc(sizeof(AudioPacket));
    packet->data = malloc(length);
    memcpy(packet->data, buffer, length);
    packet->length = length / sizeof(packet->data[0]);
    g_idle_add_once(gui_idle_callback, packet);

    // Simulate being muted
    if (muted)
        memset(buffer, 0, length);
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

        Mix_FadeInMusicPos(
            stream->music,
            0,
            FADE_DURATION_MS * playback_speed,
            stream->fade_pos
        );
    }

    else
    {
    #if !(CONTINUE_VISUALISATION_WHEN_PAUSED)
        Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
    #endif

        stream->fade_pos = Mix_GetMusicPosition(stream->music);
        Mix_FadeOutMusic(FADE_DURATION_MS * playback_speed);
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

    playback_speed = preferences_get_playback_speed();
    Mix_SetSpeed(playback_speed);

    float fps = (float)AUDIO_FREQUENCY / (float)PACKET_SIZE;
    printf("running at approx. %.2f FPS\n", fps);

}

void mute_audio()
{
    // We cannot mute music in the proper way (i.e by using SDL Mixer) as it
    // will "zero out" the data that is sent to the callback, breaking the
    // visualisation
    muted = true;
}

void unmute_audio()
{
    muted = false;
}

void set_audio_speed(float speed)
{
    playback_speed = speed;
    Mix_SetSpeed(speed);
}

void close_audio()
{
    Mix_CloseAudio();
}
