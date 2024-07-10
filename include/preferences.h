#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum VisualisationType
{
    VISUALISATION_TYPE_TIME_DOMAIN,
    VISUALISATION_TYPE_FREQUENCY_DOMAIN
} VisualisationType;

typedef struct __attribute__((__packed__)) FrequencyRange
{
    double minimum;
    double maximum;
    double multiplier;
} FrequencyRange;

void init_preferences();
void toggle_preferences_window();
void free_preferences();

int                     preferences_get_gap_size();
VisualisationType       preferences_get_visualisation_type();
bool                    preferences_get_fade_edges();
int                     preferences_get_minimum_frequency();
int                     preferences_get_maximum_frequency();
bool                    preferences_get_use_bark_scale();
float                   preferences_get_gain();
float                   preferences_get_playback_speed();
bool                    preferences_get_equaliser_enabled();
int                     preferences_get_n_frequency_ranges();
const FrequencyRange*   preferences_get_frequency_ranges();
void                    preferences_force_frequency_range_ui_update();

void preferences_set_frequency_ranges(const FrequencyRange* ranges, size_t n);
