#include "preferences.h"
#include "common.h"
#include "audio_stream.h"
#include <adwaita.h>

static GSettings* settings;
static GObject* window = NULL;

static GtkWidget* frequency_range_group;
static const FrequencyRange* frequency_ranges;
static size_t n_frequency_ranges = 0;
static size_t previous_n_ranges = 0;

static void on_reset_preferences(GtkButton*);
static void on_playback_speed_changed(GtkAdjustment*, gpointer);
static void on_settings_changed(GSettings*, gchar* key, gpointer);
static void on_equaliser_toggled(GObject* self, GParamSpec*, gpointer);
static void on_add_frequency_range(GtkButton*);
static void on_clear_frequency_ranges(GtkButton*);
static void on_frequency_range_min_changed(GtkEditable*, gpointer);
static void on_frequency_range_max_changed(GtkEditable*, gpointer);
static void on_frequency_range_multiplier_changed(GtkEditable*, gpointer);

static void add_empty_frequency_range_ui();
static void add_frequency_range_to_ui(float min, float max, float multiplier, int i);

void init_preferences()
{
    settings = g_settings_new("com.github.lukawarren.waveform");

    // Get initial frequency ranges
    GVariant* variant = g_settings_get_value(settings, "frequency-ranges");
    frequency_ranges = g_variant_get_fixed_array(variant, &n_frequency_ranges, sizeof(FrequencyRange));

    g_signal_connect(settings, "changed", G_CALLBACK(on_settings_changed), NULL);
}

void toggle_preferences_window()
{
    // Close if already open
    if (window != NULL)
    {
        gtk_window_close(GTK_WINDOW(window));
        window = NULL;
        return;
    }

    GtkBuilder* builder = gtk_builder_new_from_resource("/com/github/lukawarren/waveform/src/ui/preferences.ui");
    window = gtk_builder_get_object(builder, "preferences_window");
    gtk_window_set_modal(GTK_WINDOW(window), false);

    GtkWidget* visualisation_type       = GET_WIDGET("visualisation_type");
    GtkWidget* gap_size                 = GET_WIDGET("gap_size");
    GtkWidget* fade_edges               = GET_WIDGET("fade_edges");
    GtkWidget* minimum_frequency        = GET_WIDGET("minimum_frequency");
    GtkWidget* maximum_frequency        = GET_WIDGET("maximum_frequency");
    GtkWidget* use_bark_scale           = GET_WIDGET("use_bark_scale");
    GtkWidget* gain                     = GET_WIDGET("gain");
    GtkWidget* playback_speed           = GET_WIDGET("playback_speed");
    GtkWidget* reset_button             = GET_WIDGET("reset_button");
    GtkWidget* enable_equaliser         = GET_WIDGET("enable_equaliser");
    GtkWidget* add_frequency_range      = GET_WIDGET("add_frequency_range");
    GtkWidget* clear_frequency_ranges   = GET_WIDGET("clear_frequency_ranges");

    frequency_range_group = GET_WIDGET("frequency_range_group");
    previous_n_ranges = (size_t)-1;
    on_settings_changed(NULL, "frequency-ranges", NULL);

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

    g_settings_bind(
        settings,
        "gain",
        gain,
        "value",
        G_SETTINGS_BIND_DEFAULT
    );

    g_settings_bind(
        settings,
        "playback-speed",
        playback_speed,
        "value",
        G_SETTINGS_BIND_DEFAULT
    );

    g_settings_bind(
        settings,
        "equaliser-enabled",
        enable_equaliser,
        "active",
        G_SETTINGS_BIND_DEFAULT
    );

    GtkAdjustment* speed_adjustment = adw_spin_row_get_adjustment(ADW_SPIN_ROW(playback_speed));
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_preferences), NULL);
    g_signal_connect(speed_adjustment, "value-changed", G_CALLBACK(on_playback_speed_changed), NULL);
    g_signal_connect(enable_equaliser, "notify::active", G_CALLBACK(on_equaliser_toggled), NULL);
    g_signal_connect(add_frequency_range, "clicked", G_CALLBACK(on_add_frequency_range), NULL);
    g_signal_connect(clear_frequency_ranges, "clicked", G_CALLBACK(on_clear_frequency_ranges), NULL);
    on_equaliser_toggled(G_OBJECT(enable_equaliser), NULL, NULL);

    g_object_unref(builder);
    gtk_window_present(GTK_WINDOW(window));
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

float preferences_get_gain()
{
    return (float)g_settings_get_int(settings, "gain") / 10.0f;
}

float preferences_get_playback_speed()
{
    return (float)g_settings_get_double(settings, "playback-speed");
}

bool preferences_get_equaliser_enabled()
{
    return g_settings_get_boolean(settings, "equaliser-enabled");
}

int preferences_get_n_frequency_ranges()
{
    return n_frequency_ranges;
}

const FrequencyRange* preferences_get_frequency_ranges()
{
    return frequency_ranges;
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
        // Only reset preferences on visualisation page
        g_settings_reset(settings, "gap-size");
        g_settings_reset(settings, "visualisation-type");
        g_settings_reset(settings, "fade-edges");
        g_settings_reset(settings, "minimum-frequency");
        g_settings_reset(settings, "maximum-frequency");
        g_settings_reset(settings, "use-bark-scale");
        g_settings_reset(settings, "gain");
    }
}

static void on_reset_preferences(GtkButton*)
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

static void on_playback_speed_changed(GtkAdjustment* adjustment, gpointer)
{
    set_audio_speed(gtk_adjustment_get_value(adjustment));
}

static void on_settings_changed(GSettings*, gchar* key, gpointer)
{
    if (strcmp(key, "frequency-ranges") != 0) return;

    // Update data
    GVariant* variant = g_settings_get_value(settings, "frequency-ranges");
    frequency_ranges = g_variant_get_fixed_array(variant, &n_frequency_ranges, sizeof(FrequencyRange));

    // Avoid re-drawing UI when we don't need to
    if (previous_n_ranges != n_frequency_ranges)
    {
        // Clear UI
        GtkWidget* child = gtk_widget_get_first_child(frequency_range_group);
        child = gtk_widget_get_last_child(child);
        child = gtk_widget_get_first_child(child);
        gtk_list_box_remove_all(GTK_LIST_BOX(child));

        if (n_frequency_ranges == 0)
            add_empty_frequency_range_ui();

        for (size_t i = 0; i < n_frequency_ranges; ++i)
            add_frequency_range_to_ui(
                frequency_ranges[i].minimum,
                frequency_ranges[i].maximum,
                frequency_ranges[i].multiplier,
                i
            );
    }
    previous_n_ranges = n_frequency_ranges;
}

static void on_equaliser_toggled(GObject* self, GParamSpec*, gpointer)
{
    bool is_active = adw_switch_row_get_active(ADW_SWITCH_ROW(self));
    gtk_widget_set_sensitive(frequency_range_group, is_active);
}

static void set_frequency_ranges(FrequencyRange* ranges, size_t n)
{
    GVariantType* type = g_variant_type_new("(ddd)");
    GVariant* value = g_variant_new_fixed_array(
        type,
        ranges,
        n,
        sizeof(FrequencyRange)
    );
    g_settings_set_value(settings, "frequency-ranges", value);
    g_variant_type_free(type);
}

static void on_add_frequency_range(GtkButton*)
{
    // Copy over old array
    FrequencyRange* new_frequency_ranges = malloc(sizeof(FrequencyRange) * (n_frequency_ranges + 1));
    memcpy(new_frequency_ranges, frequency_ranges, sizeof(FrequencyRange) * n_frequency_ranges);

    // Set new element
    new_frequency_ranges[n_frequency_ranges].minimum = 0;
    new_frequency_ranges[n_frequency_ranges].maximum = 1000;
    new_frequency_ranges[n_frequency_ranges].multiplier = 0.05f;

    set_frequency_ranges(new_frequency_ranges, n_frequency_ranges + 1);
    free(new_frequency_ranges);
}

static void on_clear_frequency_ranges(GtkButton*)
{
    set_frequency_ranges(NULL, 0);
}

static void apply_frequency_range_change(GtkEditable* self, size_t index, int mode)
{
    // Make copy of list with ammendments
    FrequencyRange* new_ranges = malloc(sizeof(FrequencyRange) * n_frequency_ranges);
    memcpy(new_ranges, frequency_ranges, sizeof(FrequencyRange) * n_frequency_ranges);
    double value = adw_spin_row_get_value(ADW_SPIN_ROW(self));
    if (mode == 0) new_ranges[index].minimum = value;
    if (mode == 1) new_ranges[index].maximum = value;
    if (mode == 2) new_ranges[index].multiplier = value;

    set_frequency_ranges(new_ranges, n_frequency_ranges);
    free(new_ranges);
}

static void on_frequency_range_min_changed(GtkEditable* self, gpointer data)
{
    apply_frequency_range_change(self, (size_t)data, 0);
}

static void on_frequency_range_max_changed(GtkEditable* self, gpointer data)
{
    apply_frequency_range_change(self, (size_t)data, 1);
}

static void on_frequency_range_multiplier_changed(GtkEditable* self, gpointer data)
{
    apply_frequency_range_change(self, (size_t)data, 2);
}

static void add_empty_frequency_range_ui()
{
    GtkWidget* row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No Frequency Ranges Found");
    gtk_widget_set_sensitive(row, false);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(frequency_range_group), row);
}

static void add_frequency_range_to_ui(float min, float max, float multiplier, int i)
{
    GtkWidget* row = adw_expander_row_new();
    char* title = g_strdup_printf("Range %d", i + 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);

    GtkWidget* spin_min = adw_spin_row_new_with_range(0, 24000, 1);
    GtkWidget* spin_max = adw_spin_row_new_with_range(0, 24000, 1);
    GtkWidget* spin_multiplier = adw_spin_row_new_with_range(0, 1, 0.01);
    adw_spin_row_set_digits(ADW_SPIN_ROW(spin_multiplier), 2);

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(spin_min), "Minimum Frequency");
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(spin_max), "Maximum Frequency");
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(spin_multiplier), "Multiplier");

    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(spin_multiplier),
        "Values very close or equal to zero may introduce some minor crackling"
    );

    adw_spin_row_set_value(ADW_SPIN_ROW(spin_min), min);
    adw_spin_row_set_value(ADW_SPIN_ROW(spin_max), max);
    adw_spin_row_set_value(ADW_SPIN_ROW(spin_multiplier), multiplier);

    adw_expander_row_add_row(ADW_EXPANDER_ROW(row), spin_min);
    adw_expander_row_add_row(ADW_EXPANDER_ROW(row), spin_max);
    adw_expander_row_add_row(ADW_EXPANDER_ROW(row), spin_multiplier);

    g_signal_connect(spin_min, "changed", G_CALLBACK(on_frequency_range_min_changed), (gpointer)(size_t)i);
    g_signal_connect(spin_max, "changed", G_CALLBACK(on_frequency_range_max_changed), (gpointer)(size_t)i);
    g_signal_connect(spin_multiplier, "changed", G_CALLBACK(on_frequency_range_multiplier_changed), (gpointer)(size_t)i);

    adw_preferences_group_add(ADW_PREFERENCES_GROUP(frequency_range_group), row);
}
