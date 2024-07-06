#include "visualiser.h"
#include "preferences.h"
#include "common.h"
#include <complex.h>
#include <fftw3.h>

#define N_FRAMES 20

static float* audio_data[N_FRAMES] = {};
static int audio_length;
static int previous_frame;
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

static float hertz_to_bark_scale(float f)
{
    // From https://en.wikipedia.org/wiki/Bark_scale
    return 13.0f * atanf(0.00076f * f) +
        3.5f * atanf((f / 7500.0f) * (f / 7500.0f));
}

static float bark_to_hertz_scale(float f)
{
    // https://www.mathworks.com/help/audio/ref/bark2hz.html
    if (f < 2.0f) f = (f - 0.3f) / 0.85f;
    if (f > 20.1f) f = (f + 4.422f) / 1.22f;
    return 1960.0f * (f + 0.53f) / (26.28f - f);
}

static int frequency_to_fft_index(float frequency)
{
    if (frequency < 0 || frequency > AUDIO_FREQUENCY / 2)
        g_warning_once("invalid frequency %f", frequency);

    int index = (int)(frequency / (float)AUDIO_FREQUENCY * (float)audio_length);
    return MIN(index, audio_length - 1);
}

static float get_bar_height_from_fft(
    fftw_complex* output,
    float minimum_frequency,
    float maximum_frequency,
    float progress,
    float progress_step,
    bool use_bark_scale
)
{
    int min_index, max_index;

    // Incoming frequencies are in whatever scale we want to use
    float frequency_range = maximum_frequency - minimum_frequency;
    float min_frequency = minimum_frequency + frequency_range * progress;
    float max_frequency = minimum_frequency + frequency_range * (progress + progress_step);

    if (!use_bark_scale)
    {
        // Pick range in Hertz scale directly
        min_index = frequency_to_fft_index(min_frequency);
        max_index = frequency_to_fft_index(max_frequency);
    }
    else
    {
        // Picked range was in Bark scale; go back to Hertz
        float min_index_hertz = bark_to_hertz_scale(min_frequency);
        float max_index_hertz = bark_to_hertz_scale(max_frequency);
        min_index = frequency_to_fft_index(min_index_hertz);
        max_index = frequency_to_fft_index(max_index_hertz);
    }

    // If the resolution is such that adjacent bars will be of the
    // same index (e.g. at the lower end of frequencies), only
    // draw a single bar instead of a whole ugly "block"
    if (use_bark_scale && min_index == max_index)
        return 0.0f;

    // Average each "bin"
    double total = 0;
    for (int i = min_index; i <= max_index; ++i)
        total += cabs(output[i]);

    return total / (double)(max_index - min_index + 1);
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

    fftw_complex* fft_output = NULL;
    bool is_frequency_domain =
        preferences_get_visualisation_type() == VISUALISATION_TYPE_FREQUENCY_DOMAIN;

    if (is_frequency_domain)
    {
        double* buffer = fftw_alloc_real(audio_length);
        fft_output = fftw_alloc_complex(audio_length);

        // Convert average input audio
        for (int i = 0; i < audio_length; ++i)
            buffer[i] = get_average_frame_value(i);

        // Perform DFT
        fftw_plan plan = fftw_plan_dft_r2c_1d(
            audio_length,
            buffer,
            fft_output,
            FFTW_ESTIMATE
        );
        fftw_execute_dft_r2c(plan, buffer, fft_output);
        fftw_destroy_plan(plan);
        fftw_free(buffer);
    }

    // Drawing settings
    GdkRGBA background_colour = get_background_colour();
    GdkRGBA base_bar_colour = get_base_bar_colour();
    int step_size = preferences_get_gap_size();
    bool fade_edges = preferences_get_fade_edges();

    // FFT settings
    bool use_bark_scale = preferences_get_use_bark_scale();
    float minimum_frequency = preferences_get_minimum_frequency();
    float maximum_frequency = preferences_get_maximum_frequency();
    if (use_bark_scale)
    {
        minimum_frequency = hertz_to_bark_scale(minimum_frequency);
        maximum_frequency = hertz_to_bark_scale(maximum_frequency);
    }
    float progress_step = (float)step_size / (float)width;

    for (int i = 0; i < width; i += step_size)
    {
        // Position in audio
        float f = (float)i / (float)width;
        int progress = (int)(f * (float)audio_length);
        progress = MIN(MAX(progress, 0), audio_length - 1);

        float bar_height;
        if (is_frequency_domain)
        {
            bar_height = get_bar_height_from_fft(
                fft_output,
                minimum_frequency,
                maximum_frequency,
                f,
                progress_step,
                use_bark_scale
            );

            // Scale logarithmically
            float offset = 1.0f;
            bar_height = log10f(offset + bar_height);
            bar_height *= (float)height;
        }
        else
        {
            // Map from [-1, 1] to [0, height]
            float scale = 10.0f;
            bar_height = get_average_frame_value(progress);
            bar_height = bar_height / 2.0f * (float)height;
            bar_height *= scale;
        }


        // Colour
        if (fade_edges)
        {
            float mix_amount = 1.0f - sinf(G_PI * f);
            GdkRGBA colour = mix_colours(&base_bar_colour, &background_colour, mix_amount);
            gdk_cairo_set_source_rgba(cairo, &colour);
        }
        else
            gdk_cairo_set_source_rgba(cairo, &base_bar_colour);

        cairo_rectangle(cairo, i, height - bar_height - 1.0f, 1.0f, bar_height);
        cairo_fill(cairo);
    }

    if (fft_output != NULL)
        fftw_free(fft_output);
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

    previous_frame = current_frame;
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
