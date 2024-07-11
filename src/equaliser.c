#include "equaliser.h"
#include "preferences.h"
#include "common.h"
#include <adwaita.h>
#include <complex.h>
#include <fftw3.h>

/*
    The equaliser effect requires having access to both the previous packet
    and the next packet. Unfortunately, I cannot be bothered patching SDL_mixer
    to achieve this. Instead, let's just cache the previous two buffers, and
    live with a delay. It should only be 2 frames anyway (i.e. 30ish ms).
*/

static float* previous_packets[2];
static int n_packets = 0;

static float* samples_buffer = NULL;
static float* output_buffer = NULL;
static fftwf_complex* left_fft = NULL;
static fftwf_complex* right_fft = NULL;
static float* left_ifft = NULL;
static float* right_ifft = NULL;

static float* apply_equaliser(float* previous, float* current, float* next);
static void modify_frequency_range(float min, float max, float multipiler);
static float complex modify_magnitude(float complex c, float multiplier);

void equaliser_init()
{
    samples_buffer = malloc(sizeof(float) * PACKET_SIZE * CHANNELS * 3);
    left_fft = fftwf_alloc_complex(PACKET_SIZE * 3);
    right_fft = fftwf_alloc_complex(PACKET_SIZE * 3);
    left_ifft = fftwf_alloc_real(PACKET_SIZE * 3);
    right_ifft = fftwf_alloc_real(PACKET_SIZE * 3);
    output_buffer = malloc(sizeof(float) * PACKET_SIZE * CHANNELS);
}

void equaliser_process_packet(AudioPacket* packet)
{
    g_assert(packet->length / CHANNELS == PACKET_SIZE);
    size_t size = sizeof(packet->data[0]) * packet->length;

    // If not "satiated" yet, fill buffer
    if (n_packets != 2)
    {
        previous_packets[n_packets] = malloc(size);
        memcpy(previous_packets[n_packets], packet->data, size);
        n_packets++;

        memset(packet->data, 0, size);
        return;
    }

    float* result = apply_equaliser(
        previous_packets[0],
        previous_packets[1],
        packet->data
    );

    // Shift packets along
    memcpy(previous_packets[0], previous_packets[1], size);
    memcpy(previous_packets[1], packet->data, size);
    memcpy(packet->data, result, size);
}

static float* apply_equaliser(float* previous, float* current, float* next)
{
    /*
        We want to join together the previous, current and next packets,
        whilst also keeping the channels separate.

        ----------------
        | left | right |
        ----------------
           |
           v
        -----------------------------
        | previous | current | next |
        -----------------------------
    */

    g_assert(CHANNELS == 2);

    // Fill samples buffer
    #define FILL for (int i = 0; i < PACKET_SIZE; ++i)
    FILL samples_buffer[PACKET_SIZE * 0 + i] = previous[i * 2]; // left ear previous
    FILL samples_buffer[PACKET_SIZE * 1 + i] = current[i * 2]; // left ear current
    FILL samples_buffer[PACKET_SIZE * 2 + i] = next[i * 2]; // left ear next
    FILL samples_buffer[PACKET_SIZE * 3 + i] = previous[i * 2 + 1]; // right ear previous
    FILL samples_buffer[PACKET_SIZE * 4 + i] = current[i * 2 + 1]; // right ear current
    FILL samples_buffer[PACKET_SIZE * 5 + i] = next[i * 2 + 1]; // right ear next

    // Perform FFT for each channel
    fftwf_plan plan_left = fftwf_plan_dft_r2c_1d(
        PACKET_SIZE * 3,
        samples_buffer,
        left_fft,
        FFTW_ESTIMATE
    );
    fftwf_plan plan_right = fftwf_plan_dft_r2c_1d(
        PACKET_SIZE * 3,
        samples_buffer + PACKET_SIZE * 3,
        right_fft,
        FFTW_ESTIMATE
    );
    fftwf_execute_dft_r2c(plan_left, samples_buffer, left_fft);
    fftwf_execute_dft_r2c(plan_right, samples_buffer + PACKET_SIZE * 3, right_fft);
    fftwf_destroy_plan(plan_left);
    fftwf_destroy_plan(plan_right);

    // Modify audio in time domain
    int n_ranges = preferences_get_n_frequency_ranges();
    const FrequencyRange* ranges = preferences_get_frequency_ranges();
    for (int i = 0;  i < n_ranges; ++i)
        modify_frequency_range(
            ranges[i].minimum,
            ranges[i].maximum,
            ranges[i].multiplier
        );

    // Convert back to frequency domain
    fftwf_plan inverse_plan_left = fftwf_plan_dft_c2r_1d(
        PACKET_SIZE * 3,
        left_fft,
        left_ifft,
        FFTW_ESTIMATE
    );
    fftwf_plan inverse_plan_right = fftwf_plan_dft_c2r_1d(
        PACKET_SIZE * 3,
        right_fft,
        right_ifft,
        FFTW_ESTIMATE
    );
    fftwf_execute_dft_c2r(inverse_plan_left, left_fft, left_ifft);
    fftwf_execute_dft_c2r(inverse_plan_right, right_fft, right_ifft);
    fftwf_destroy_plan(inverse_plan_left);
    fftwf_destroy_plan(inverse_plan_right);

    // Retrieve processed samples
    FILL output_buffer[i * 2 + 0] = left_ifft[PACKET_SIZE + i] / (float)PACKET_SIZE / 3.0f;
    FILL output_buffer[i * 2 + 1] = right_ifft[PACKET_SIZE + i] / (float)PACKET_SIZE / 3.0f;
    return output_buffer;
}

static void modify_frequency_range(float min, float max, float multiplier)
{
    /*
        Work out bins.

        Because of the finite bin size, the lower bin may not be 0, even if the
        frequency is. Likewise, the upper bin may not be the maximum it can be
        even if the upper frequency is 20,000 Hz!

        Adding a bit of leeway helps fix this for when the lower frequency is
        0 Hz, for example.
    */

    float frequency_resolution = (float)AUDIO_FREQUENCY / (float)(PACKET_SIZE * 3);
    int lower_bin = (int)(min / frequency_resolution);
    int upper_bin = (int)(max / frequency_resolution);
    lower_bin = MAX(lower_bin - 1, 0);
    upper_bin = MIN(upper_bin + 1, PACKET_SIZE * 3 - 1);

    for (int i = lower_bin; i <= upper_bin; ++i)
    {
        left_fft[i] = modify_magnitude(left_fft[i], multiplier);
        right_fft[i] = modify_magnitude(right_fft[i], multiplier);
    }
}

static float complex modify_magnitude(float complex c, float multiplier)
{
    // Convert to polar form
    float magnitude = cabsf(c);
    float phase = cargf(c);
    magnitude *= multiplier;

    // Convert back to Cartesian form
    c = magnitude * cosf(phase) + I * magnitude * sinf(phase);
    return c;
}

void equaliser_destroy()
{
    if (previous_packets[0] != NULL) free(previous_packets[0]);
    if (previous_packets[1] != NULL) free(previous_packets[1]);

    free(samples_buffer);
    fftwf_free(left_fft);
    fftwf_free(right_fft);
    fftwf_free(left_ifft);
    fftwf_free(right_ifft);
    free(output_buffer);
}
