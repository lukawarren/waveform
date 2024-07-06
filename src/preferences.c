#include "preferences.h"
#include "common.h"
#include <adwaita.h>

static GSettings* settings;

void init_preferences()
{
    settings = g_settings_new("com.github.lukawarren.waveform");
}

void create_preferences_window()
{
    GtkBuilder* builder = gtk_builder_new_from_resource("/com/github/lukawarren/waveform/src/ui/preferences.ui");
    GObject* window = gtk_builder_get_object(builder, "preferences_window");
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    GtkWidget* visualisation_type   = GET_WIDGET("visualisation_type");
    GtkWidget* gap_size             = GET_WIDGET("gap_size");
    GtkWidget* minimum_frequency    = GET_WIDGET("minimum_frequency");
    GtkWidget* maximum_frequency    = GET_WIDGET("maximum_frequency");

    g_settings_bind(
        settings,
        "visualisation-type",
        visualisation_type,
        "selected",
        G_SETTINGS_BIND_DEFAULT
    );

    g_settings_bind(
        settings,
        "gap-size",
        gap_size,
        "value",
        G_SETTINGS_BIND_DEFAULT
    );

    g_settings_bind(
        settings,
        "minimum-frequency",
        minimum_frequency,
        "value",
        G_SETTINGS_BIND_DEFAULT
    );

    g_settings_bind(
        settings,
        "maximum-frequency",
        maximum_frequency,
        "value",
        G_SETTINGS_BIND_DEFAULT
    );

    g_object_unref(builder);
}

void free_preferences()
{
    g_object_unref(settings);
}

int preferences_get_gap_size()
{
    return g_settings_get_int(settings, "gap-size");
}

VisualisationType preferences_get_visualisation_type()
{
    if (g_settings_get_int(settings, "visualisation-type") == 0)
        return VISUALISATION_TYPE_FREQUENCY_DOMAIN;
    else
        return VISUALISATION_TYPE_TIME_DOMAIN;
}

int preferences_get_minimum_frequency()
{
    return g_settings_get_int(settings, "minimum-frequency");
}

int preferences_get_maximum_frequency()
{
    return g_settings_get_int(settings, "maximum-frequency");
}
