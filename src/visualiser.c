#include "visualiser.h"
#include "preferences.h"
#include "common.h"
#include <complex.h>
#include <fftw3.h>

#define FRAME_SIZE PACKET_SIZE
#define N_FRAMES 5

static float* audio_data;
static float* processed_frames[N_FRAMES] = {};
static int current_frame = 0;

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

    int index = (int)(frequency / (float)AUDIO_FREQUENCY * (float)FRAME_SIZE);
    return MIN(index, FRAME_SIZE - 1);
}

static void add_fft_frame()
{
    float* buffer = fftwf_alloc_real(FRAME_SIZE);
    fftwf_complex* fft_output = fftwf_alloc_complex(FRAME_SIZE);

    // Feed input audio
    for (int i = 0; i < FRAME_SIZE; ++i)
        buffer[i] = audio_data[i];

    // Perform DFT
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(
        FRAME_SIZE,
        buffer,
        fft_output,
        FFTW_ESTIMATE
    );
    fftwf_execute_dft_r2c(plan, buffer, fft_output);

    // Add to frame
    for (int i = 0; i < FRAME_SIZE; ++i)
        processed_frames[current_frame][i] = cabsf(fft_output[i]);
    current_frame = (current_frame + 1) % N_FRAMES;

    // Free
    fftwf_destroy_plan(plan);
    fftwf_free(buffer);
    fftwf_free(fft_output);
}

static void add_time_domain_frame()
{
    // Add to frame, but take absolute value as audio is signed
    for (int i = 0; i < FRAME_SIZE; ++i)
        processed_frames[current_frame][i] = fabs(audio_data[i]);
    current_frame = (current_frame + 1) % N_FRAMES;
}

static float get_bar_height_from_fft(
    float* frame,
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

#if BARK_SCALE_HIDE_ADJACENT_BARS
    // If the resolution is such that adjacent bars will be of the
    // same index (e.g. at the lower end of frequencies), only
    // draw a single bar instead of a whole ugly "block"
    if (use_bark_scale && min_index == max_index)
        return 0.0f;
#endif

    // Average each "bin"
    float total = 0;
    for (int i = min_index; i <= max_index; ++i)
        total += frame[i];

    return total / (float)(max_index - min_index + 1);
}

static float get_bar_height(
    float progress,
    float progress_step,
    float minimum_frequency,
    float maximum_frequency,
    float gain,
    float height,
    bool use_bark_scale,
    bool is_frequency_domain
)
{
    if (is_frequency_domain)
    {
        float total = 0.0f;
        for (int i = 0; i < N_FRAMES; ++i)
        {
            total += get_bar_height_from_fft(
                processed_frames[i],
                minimum_frequency,
                maximum_frequency,
                progress,
                progress_step,
                use_bark_scale
            );
        }
        total /= (float)N_FRAMES;

        // Scale logarithmically
        total = log10f(gain + total);
        total *= (float)height;
        return total;
    }
    else
    {
        int index = (int)(progress * (float)FRAME_SIZE);
        index = MIN(MAX(index, 0), FRAME_SIZE - 1);

        // Average raw audio samples over N frames
        float total = 0.0f;
        for (int i = 0; i < N_FRAMES; ++i)
            total += processed_frames[i][index];
        total /= (float)N_FRAMES;

        // Map from [-1, 1] to [0, height]
        float scale = 10.0f;
        float bar_height = total / 2.0f * (float)height;
        bar_height *= scale;
        return bar_height;
    }
}

void visualiser_draw_function(
    GtkDrawingArea*,
    cairo_t*        cairo,
    int             width,
    int             height,
    gpointer
)
{
    bool is_frequency_domain =
        preferences_get_visualisation_type() == VISUALISATION_TYPE_FREQUENCY_DOMAIN;

    if (is_frequency_domain)
        add_fft_frame();
    else
        add_time_domain_frame();

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
    float gain = preferences_get_gain();

    for (int i = 0; i < width; i += step_size)
    {
        // Position in frame
        float progress = (float)i / (float)width;

        // Sample frame(s)
        float bar_height = get_bar_height(
            progress,
            progress_step,
            minimum_frequency,
            maximum_frequency,
            gain,
            height,
            use_bark_scale,
            is_frequency_domain
        );

        // Colour
        if (fade_edges)
        {
            float mix_amount = 1.0f - sinf(G_PI * progress);
            GdkRGBA colour = mix_colours(&base_bar_colour, &background_colour, mix_amount);
            gdk_cairo_set_source_rgba(cairo, &colour);
        }
        else
            gdk_cairo_set_source_rgba(cairo, &base_bar_colour);

        cairo_rectangle(cairo, i, height - bar_height - 1.0f, 1.0f, bar_height);
        cairo_fill(cairo);
    }
}

static void on_theme_changed(GObject*, GParamSpec*, gpointer data)
{
#ifndef __APPLE__
    bool is_dark_mode;
    GtkSettings* settings = gtk_settings_get_default();
    g_object_get(G_OBJECT(settings), "gtk-application-prefer-dark-theme", &is_dark_mode, NULL);

    const char* classes[] = {
        is_dark_mode ? "fully-bright-colour" : "fully-dark-colour",
        NULL
    };
#else
    // Dark-mode unsupported (see main.c) - TODO: use AdwStyleManager
    const char* classes[] = {
        "fully-dark-colour",
        NULL
    };
#endif

    GtkWidget* widget = (GtkWidget*)data;
    gtk_widget_set_css_classes(widget, classes);
}

void visualiser_init(GtkWidget* widget)
{
    // Allocate buffers
    audio_data = calloc(FRAME_SIZE, sizeof(float));
    for (int i = 0; i < N_FRAMES; ++i)
        processed_frames[i] = calloc(FRAME_SIZE, sizeof(float));

    // Setup UI - macOS dark mode stubbed unsupported for now
#ifndef __APPLE__
    GtkSettings* settings = gtk_settings_get_default();
    g_signal_connect(settings, "notify::gtk-application-prefer-dark-theme", G_CALLBACK(on_theme_changed), widget);
#endif
    on_theme_changed(NULL, NULL, widget);
}

void visualiser_set_data(AudioPacket* packet)
{
    g_assert(packet->length / CHANNELS == PACKET_SIZE);
    g_assert(PACKET_SIZE == FRAME_SIZE);

    // Average over each channel
    for (int i = 0; i < packet->length / CHANNELS; ++i)
    {
        float sum = 0.0f;
        for (int c = 0; c < CHANNELS; ++c)
            sum += packet->data[i * CHANNELS + c];

        audio_data[i] = sum / (float)CHANNELS;
    }
}

void visualiser_free_data()
{
    free(audio_data);
    for (int i = 0; i < N_FRAMES; ++i)
        free(processed_frames[i]);
}
