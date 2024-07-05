#include "visualiser.h"

#define N_FRAMES 10

static float* audio_data[N_FRAMES] = {};
static int audio_length;
static int current_frame = 0;

static int get_occupied_frames()
{
    for (int i = 0; i < N_FRAMES; ++i)
        if (audio_data[i] == NULL)
            return i;

    return N_FRAMES;
}

static float get_average_frame_value(int index)
{
    int frames = get_occupied_frames();
    float sum = 0.0f;
    for (int i = 0; i < frames; ++i)
    {
        // If we average an entire song we would expect the result to be
        // about zero, so it'll look visually better if we abs everything
        float value = audio_data[i][index];
        sum += fabs(value);
    }
    return sum / (float)frames;
}

void visualiser_draw_function(
    GtkDrawingArea* area,
    cairo_t*        cairo,
    int             width,
    int             height,
    gpointer
)
{
    if (get_occupied_frames() == 0)
        return;

    int step_size = 5;
    float scale = 10.0f;

    for (int i = 0; i < width; i += step_size)
    {
        // Position in audio
        float f = (float)i / (float)width;
        int progress = (int)(f * (float)audio_length);
        progress = MIN(MAX(progress, 0), audio_length - 1);

        // Bar height
        float bar_height = get_average_frame_value(progress);
        bar_height = bar_height / 2.0f * (float)height;
        bar_height *= scale;

        // Colour
        GdkRGBA colour;
        colour.alpha = 1.0f;
        colour.red = colour.blue = colour.green = 1.0f - sinf(G_PI * f);
        gdk_cairo_set_source_rgba(cairo, &colour);
        cairo_rectangle(cairo, i, height - bar_height - 1.0f, 1.0f, bar_height);
        cairo_fill(cairo);
    }
}

void visualiser_set_data(AudioPacket* packet)
{
    // Delete old frame
    if (audio_data[current_frame] != NULL)
        free(audio_data[current_frame]);

    int channels = 2;
    audio_data[current_frame] = malloc(sizeof(packet->data[0]) * packet->length / channels);

    // Average over both channels
    for (int i = 0; i < packet->length / channels; i++)
        audio_data[current_frame][i] = (packet->data[i * 2] + packet->data[i * 2 + 1]) / 2.0f;

    // Each packet should have an identical length
    // so there is no need to check they all match
    audio_length = packet->length / channels;

    current_frame = (current_frame + 1) % N_FRAMES;
}

void visualiser_free_data()
{
    for (int i = 0; i < N_FRAMES; ++i)
    {
        if (audio_data[i] != NULL)
        {
            free(audio_data[i]);
            audio_data[i] = NULL;
        }
    }
}