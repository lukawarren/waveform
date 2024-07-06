#pragma once

#define GET_WIDGET(x) GTK_WIDGET(gtk_builder_get_object(builder, x))

// Audio stream settings
#define AUDIO_FREQUENCY 48000
#define TARGET_FPS 60
#define PACKET_SIZE (AUDIO_FREQUENCY / TARGET_FPS)
#define CHANNELS 2

// Leads to smooth "playback paused" animation at the cost of idle CPU usage
#define CONTINUE_VISUALISATION_WHEN_PAUSED 1