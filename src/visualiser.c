#include "visualiser.h"
#include "preferences.h"

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

static GdkRGBA mix_colours(const GdkRGBA* one, const GdkRGBA* two, float t)
{
    GdkRGBA result;
    result.red = one->red + t * (two->red - one->red);
    result.green = one->green + t * (two->green - one->green);
    result.blue = one->blue + t * (two->blue - one->blue);
    result.alpha = one->alpha + t * (two->alpha - one->alpha);
    return result;
}

static GdkRGBA get_background_colour()
{
    GdkRGBA background_colour;
    background_colour.alpha = 0.0f;
    background_colour.red = 0.0f;
    background_colour.green = 0.0f;
    background_colour.blue = 0.0f;
    return background_colour;
}

static GdkRGBA get_base_bar_colour()
{
    // Overridden by CSS
    GdkRGBA base_bar_colour;
    base_bar_colour.alpha = 1.0f;
    base_bar_colour.red = 1.0f;
    base_bar_colour.green = 1.0f;
    base_bar_colour.blue = 1.0f;

    return base_bar_colour;
}

void visualiser_draw_function(
    GtkDrawingArea*,
    cairo_t*        cairo,
    int             width,
    int             height,
    gpointer
)
{
    if (get_occupied_frames() == 0)
        return;

    if (preferences_get_visualisation_type() != VISUALISATION_TYPE_TIME_DOMAIN)
        return;

    GdkRGBA background_colour = get_background_colour();
    GdkRGBA base_bar_colour = get_base_bar_colour();

    int step_size = preferences_get_gap_size();
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
        float mix_amount = 1.0f - sinf(G_PI * f);
        GdkRGBA colour = mix_colours(&base_bar_colour, &background_colour, mix_amount);
        gdk_cairo_set_source_rgba(cairo, &colour);
        cairo_rectangle(cairo, i, height - bar_height - 1.0f, 1.0f, bar_height);
        cairo_fill(cairo);
    }
}

static void on_theme_changed(GObject*, GParamSpec*, gpointer data)
{
    GtkSettings* settings = gtk_settings_get_default();
    bool is_dark_mode;
    g_object_get(G_OBJECT(settings), "gtk-application-prefer-dark-theme", &is_dark_mode, NULL);

    GtkWidget* widget = (GtkWidget*)data;

    const char** classes = malloc(sizeof(char*) * 2);

    if (is_dark_mode)
        classes[0] = "fully-bright-colour";
    else
        classes[0] = "fully-dark-colour";

    classes[1] = NULL;
    gtk_widget_set_css_classes(widget, classes);
    free(classes);
}

void visualiser_init(GtkWidget* widget)
{
    GtkSettings* settings = gtk_settings_get_default();
    g_signal_connect(settings, "notify::gtk-application-prefer-dark-theme", G_CALLBACK(on_theme_changed), widget);
    on_theme_changed(NULL, NULL, widget);
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
