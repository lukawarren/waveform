#include "visualiser.h"

static float* audio_data;
static int audio_length;

void visualiser_draw_function(
    GtkDrawingArea* area,
    cairo_t*        cairo,
    int             width,
    int             height,
    gpointer        data
)
{
    if (audio_data == NULL)
        return;

    GdkRGBA bar_colour;
    gtk_widget_get_color(GTK_WIDGET(area), &bar_colour);
    gdk_cairo_set_source_rgba(cairo, &bar_colour);

    for (int i = 0; i < width; ++i)
    {
        int progress = (int)((float)i / (float)width * (float)audio_length);
        progress = MIN(MAX(progress, 0), audio_length - 1);
        float bar_height = (audio_data[progress] + 1.0f) / 2.0f * (float)height;

        cairo_rectangle(cairo, i, height - bar_height - 1.0f, 1.0f, bar_height);
    }


    cairo_fill(cairo);
}

void visualiser_set_data(AudioPacket* packet)
{
    visualiser_free_data();
    audio_data = malloc(sizeof(packet->data[0]) * packet->length);
    memcpy(audio_data, packet->data, sizeof(packet->data[0]) * packet->length);
    audio_length = packet->length;
}

void visualiser_free_data()
{
    if (audio_data != NULL)
        free(audio_data);
}