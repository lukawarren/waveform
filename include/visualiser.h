#pragma once
#include <adwaita.h>
#include "audio_stream.h"

void visualiser_draw_function(
    GtkDrawingArea* area,
    cairo_t*        cairo,
    int             width,
    int             height,
    gpointer        data
);

void visualiser_init(GtkWidget* widget);
void visualiser_set_data(AudioPacket* packet);
void visualiser_free_data();