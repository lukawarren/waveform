#include "preferences.h"
#include "common.h"
#include <adwaita.h>

static GSettings* settings;
static GObject* window;

static void on_reset_preferences();

void init_preferences()
{
    settings = g_settings_new("com.github.lukawarren.waveform");
}

void create_preferences_window()
{
    GtkBuilder* builder = gtk_builder_new_from_resource("/com/github/lukawarren/waveform/src/ui/preferences.ui");
    window = gtk_builder_get_object(builder, "preferences_window");
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    GtkWidget* visualisation_type   = GET_WIDGET("visualisation_type");
    GtkWidget* gap_size             = GET_WIDGET("gap_size");
    GtkWidget* fade_edges           = GET_WIDGET("fade_edges");
    GtkWidget* minimum_frequency    = GET_WIDGET("minimum_frequency");
    GtkWidget* maximum_frequency    = GET_WIDGET("maximum_frequency");
    GtkWidget* use_bark_scale       = GET_WIDGET("use_bark_scale");
    GtkWidget* reset_button         = GET_WIDGET("reset_button");

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
        "fade-edges",
        fade_edges,
        "active",
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

    g_settings_bind(
        settings,
        "use-bark-scale",
        use_bark_scale,
        "active",
        G_SETTINGS_BIND_DEFAULT
    );

    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_preferences), NULL);

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

bool preferences_get_fade_edges()
{
    return g_settings_get_boolean(settings, "fade-edges");
}

int preferences_get_minimum_frequency()
{
    return g_settings_get_int(settings, "minimum-frequency");
}

int preferences_get_maximum_frequency()
{
    return g_settings_get_int(settings, "maximum-frequency");
}

bool preferences_get_use_bark_scale()
{
    return g_settings_get_boolean(settings, "use-bark-scale");
}

static void on_reset_confirmed(GObject* self, GAsyncResult* result, gpointer)
{
    int index = gtk_alert_dialog_choose_finish(
        GTK_ALERT_DIALOG(self),
        result,
        NULL
    );

    if (index == 1)
    {
        g_settings_reset(settings, "gap-size");
        g_settings_reset(settings, "visualisation-type");
        g_settings_reset(settings, "fade-edges");
        g_settings_reset(settings, "minimum-frequency");
        g_settings_reset(settings, "maximum-frequency");
        g_settings_reset(settings, "use-bark-scale");
    }
}

static void on_reset_preferences()
{
    GtkAlertDialog* dialog = gtk_alert_dialog_new("Are you sure you want to reset all preferences?");

    const char* const buttons[] = {
        "No",
        "Yes",
        NULL
    };

    gtk_alert_dialog_set_buttons(dialog, buttons);
    gtk_alert_dialog_set_default_button(dialog, 0);

    gtk_alert_dialog_choose(
        dialog,
        GTK_WINDOW(window),
        NULL,
        on_reset_confirmed,
        NULL
    );
    g_object_unref(dialog);
}
