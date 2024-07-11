#pragma once

#define GET_WIDGET(x) GTK_WIDGET(gtk_builder_get_object(builder, x))

// Audio stream settings
#define AUDIO_FREQUENCY 48000
#define TARGET_FPS 60
#define PACKET_SIZE (AUDIO_FREQUENCY / TARGET_FPS)
#define CHANNELS 2
#define FADE_DURATION_MS 200

// Leads to smooth "playback paused" animation at the cost of idle CPU usage
#define CONTINUE_VISUALISATION_WHEN_PAUSED 0

// Avoid the Bark scale resulting in some bars being wider than others by
// hiding adjacent lines that refer to the same frequency
#define BARK_SCALE_HIDE_ADJACENT_LINES 0
