#include "presets.h"
#include "preferences.h"

static void on_preset_chosen(GSimpleAction* action, GVariant*, gpointer);

typedef struct Preset
{
    const char* name;
    const char* action_name;
} Preset;

static const Preset presets[] =
{
    { "Bass Boost", "bass_boost" },
    { "Remove Bass", "remove_bass" },
    { "Remove Treble", "remove_treble" },
    { "Low Quality Speakers", "low_quality_speakers" },
};

void init_presets_menu(GtkMenuButton* menu_button)
{
    // Create menu
    GMenu* menu = g_menu_new();
    for (size_t i = 0; i < G_N_ELEMENTS(presets); ++i)
    {
        char* action = g_strdup_printf("preset.%s", presets[i].action_name);
        g_menu_append(menu, presets[i].name, action);
        free(action);
    }

    // Create actions
    GSimpleActionGroup* group = g_simple_action_group_new();
    for (size_t i = 0; i < G_N_ELEMENTS(presets); ++i)
    {
        GSimpleAction* action = g_simple_action_new(presets[i].action_name, NULL);
        g_signal_connect(action, "activate", G_CALLBACK(on_preset_chosen), NULL);
        g_simple_action_set_enabled(action, true);
        g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
    }

    gtk_widget_insert_action_group(
        GTK_WIDGET(menu_button),
        "preset",
        G_ACTION_GROUP(group)
    );

    gtk_menu_button_set_menu_model(menu_button, G_MENU_MODEL(menu));
}

static void on_preset_chosen(GSimpleAction* action, GVariant*, gpointer)
{
    const char* name = g_action_get_name(G_ACTION(action));
    preferences_force_frequency_range_ui_update();

    if (strcmp(name, "bass_boost") == 0)
    {
        FrequencyRange* ranges = malloc(sizeof(FrequencyRange) * 1);
        ranges[0].minimum = 500.0f;
        ranges[0].maximum = 24000.0f;
        ranges[0].multiplier = 0.1f;
        preferences_set_frequency_ranges(ranges, 1);
        return;
    }

    if (strcmp(name, "remove_bass") == 0)
    {
        FrequencyRange* ranges = malloc(sizeof(FrequencyRange) * 1);
        ranges[0].minimum = 0.0f;
        ranges[0].maximum = 1000.0f;
        ranges[0].multiplier = 0.05f;
        preferences_set_frequency_ranges(ranges, 1);
        return;
    }

    if (strcmp(name, "remove_treble") == 0)
    {
        FrequencyRange* ranges = malloc(sizeof(FrequencyRange) * 1);
        ranges[0].minimum = 6000.0f;
        ranges[0].maximum = 24000.0f;
        ranges[0].multiplier = 0.05f;
        preferences_set_frequency_ranges(ranges, 1);
        return;
    }

    if (strcmp(name, "low_quality_speakers") == 0)
    {
        FrequencyRange* ranges = malloc(sizeof(FrequencyRange) * 2);
        ranges[0].minimum = 0.0f;
        ranges[0].maximum = 1000.0f;
        ranges[0].multiplier = 0.05f;
        ranges[1].minimum = 5000.0f;
        ranges[1].maximum = 24000.0f;
        ranges[1].multiplier = 0.05f;
        preferences_set_frequency_ranges(ranges, 2);
        return;
    }

    g_critical("unknown preset %s", name);
}
