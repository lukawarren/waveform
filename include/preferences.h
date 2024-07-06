#pragma once

typedef enum VisualisationType
{
    VISUALISATION_TYPE_TIME_DOMAIN,
    VISUALISATION_TYPE_FREQUENCY_DOMAIN
} VisualisationType;

void init_preferences();
void create_preferences_window();
void free_preferences();

int                 preferences_get_gap_size();
VisualisationType   preferences_get_visualisation_type();
int                 preferences_get_minimum_frequency();
int                 preferences_get_maximum_frequency();
