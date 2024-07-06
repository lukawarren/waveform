#pragma once

#define GET_WIDGET(x) GTK_WIDGET(gtk_builder_get_object(builder, x))
#define AUDIO_FREQUENCY 48000
#define TARGET_FPS 60