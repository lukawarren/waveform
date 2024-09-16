#include "dbus.h"
#include "preferences.h"
#include "playback.h"
#include <gio/gio.h>
#include <adwaita.h>

/*
    Loosely implements MPRIS - https://specifications.freedesktop.org/mpris-spec/2.2/.
    Tested on GNOME but not guaranteed to work on KDE, etc. as various stubs.
*/

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer);

static void on_method_call(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
);

static GVariant* on_get_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GError** error,
    gpointer user_data
);

static GVariant* new_metadata_string(const char* key, const char* value);
static GVariant* get_metadata_for_current_entry();
static GVariant* get_playback_status_variant();

static const GDBusInterfaceVTable interface_vtable =
{
    on_method_call,
    on_get_property,
    NULL,
    { 0 }
};

static guint owner_id = 0;
static const PlaylistEntry* current_entry = NULL;
static GDBusConnection* connection;

void init_dbus()
{
    // Create D-Bus connection
    owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        "org.mpris.MediaPlayer2.waveform",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        NULL,
        NULL,
        NULL,
        NULL
    );
}

static void on_bus_acquired(GDBusConnection* _connection, const gchar*, gpointer)
{
    connection = _connection;

    // Load introspection XML
    GBytes* resource = g_resources_lookup_data(
        "/com/github/lukawarren/waveform/waveform.dbus.xml",
        G_RESOURCE_LOOKUP_FLAGS_NONE,
        NULL
    );
    if (resource == NULL)
    {
        g_critical("failed to load D-Bus introspection XML");
        return;
    }

    // Create D-Bus node
    const char* resource_string = g_bytes_get_data(resource, NULL);
    GDBusNodeInfo* info = g_dbus_node_info_new_for_xml(resource_string, NULL);
    if (info == NULL)
    {
        g_critical("failed to create D-Bus node");
        return;
    }

    // Register object
    guint objects[] =
    {
        g_dbus_connection_register_object(
            connection,
            "/org/mpris/MediaPlayer2",
            info->interfaces[0],
            &interface_vtable,
            NULL,
            NULL,
            NULL
        ),
        g_dbus_connection_register_object(
            connection,
            "/org/mpris/MediaPlayer2",
            info->interfaces[1],
            &interface_vtable,
            NULL,
            NULL,
            NULL
        )
    };
    if (objects[0] == 0 || objects[1] == 0)
    {
        g_critical("failed to register D-Bus object(s)");
        return;
    }

    g_bytes_unref(resource);
    g_dbus_node_info_unref(info);
}

static void on_method_call(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
)
{
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)parameters;
    (void)invocation;
    (void)user_data;

    if (strcmp(interface_name, "org.mpris.MediaPlayer2.Player") == 0)
    {
        if (strcmp(method_name, "Previous") == 0)
            playback_previous();

        else if (strcmp(method_name, "Next") == 0)
            playback_next();

        else if (strcmp(method_name, "PlayPause") == 0)
            toggle_playback();

        else
            g_warning("unsupported method %s\n", method_name);
    }
    else
        g_warning("unsupported interface %s\n", interface_name);
}

static GVariant* on_get_property(
    GDBusConnection* connection,
    const gchar* sender,
    const gchar* object_path,
    const gchar* interface_name,
    const gchar* property_name,
    GError** error,
    gpointer user_data
)
{
    #define PROPERTY(x) (strcmp(property_name, x) == 0)

    (void)connection;
    (void)sender;
    (void)object_path;
    (void)error;
    (void)user_data;

    if (strcmp(interface_name, "org.mpris.MediaPlayer2") == 0)
    {
        if (PROPERTY("CanQuit"))
            return g_variant_new_boolean(true);

        if (PROPERTY("CanRaise"))
            return g_variant_new_boolean(false);

        if (PROPERTY("HasTrackList"))
            return g_variant_new_boolean(false);

        if (PROPERTY("Identity"))
            return g_variant_new_string("Waveform");

        if (PROPERTY("SupportedUriSchemes"))
            return g_variant_new_string("file");

        if (PROPERTY("SupportedMimeTypes"))
            return g_variant_new_string("audio/*");

        g_warning("unsupported property %s", property_name);
        return NULL;
    }

    else if (strcmp(interface_name, "org.mpris.MediaPlayer2.Player") == 0)
    {
        // TODO: add signal on change
        if (PROPERTY("PlaybackStatus"))
            return get_playback_status_variant();

        // TODO: add signal on change
        if (PROPERTY("Rate"))
            return g_variant_new_double(1.0);

        if (PROPERTY("MinimumRate"))
            return g_variant_new_double(1.0);

        if (PROPERTY("MaximumRate"))
            return g_variant_new_double(1.0);

        // TODO: add signal on change
        if (PROPERTY("Volume"))
            return g_variant_new_double(1.0);

        if (PROPERTY("Metadata"))
            return get_metadata_for_current_entry();

        if (PROPERTY("Position"))
            return g_variant_new_uint16(0);

        if (PROPERTY("CanGoNext"))
            return g_variant_new_boolean(true);

        if (PROPERTY("CanGoPrevious"))
            return g_variant_new_boolean(true);

        if (PROPERTY("CanPlay"))
            return g_variant_new_boolean(true);

        if (PROPERTY("CanPause"))
            return g_variant_new_boolean(true);

        if (PROPERTY("CanSeek"))
            return g_variant_new_boolean(false);

        if (PROPERTY("CanControl"))
            return g_variant_new_boolean(true);

        g_warning("unsupported property %s", property_name);
        return NULL;
    }

    else
    {
        g_warning("unknown D-Bus interface %s", interface_name);
        return NULL;
    }
}

static GVariant* new_metadata_string(const char* key, const char* value)
{
    return g_variant_new_dict_entry(
        g_variant_new_string(key),
        g_variant_new_variant(g_variant_new_string(value))
    );
}

static GVariant* get_metadata_for_current_entry()
{
    if (current_entry == NULL)
    {
        GVariant* entries[] = {
            new_metadata_string("mpris:trackid", "/org/mpris/MediaPlayer2/CurrentTrack")
        };
        return g_variant_new_array(G_VARIANT_TYPE("{sv}"), entries, 1);
    }
    else
    {
        GVariant* artist_string = g_variant_new_string(current_entry->unescaped_artist);
        GVariant* artist_array = g_variant_new_array(G_VARIANT_TYPE_STRING, &artist_string, 1);

        GVariant* entries[] = {
            new_metadata_string("mpris:trackid", "/org/mpris/MediaPlayer2/CurrentTrack"),
            new_metadata_string("xesam:title", current_entry->unescaped_name),
            g_variant_new_dict_entry(
                g_variant_new_string("xesam:artist"),
                g_variant_new_variant(artist_array)
            )
        };

        return g_variant_new_array(G_VARIANT_TYPE("{sv}"), entries, 3);
    }
}

static GVariant* get_playback_status_variant()
{
    if (current_entry == NULL)
        return g_variant_new_string("Stopped");

    else if (playback_is_playing())
        return g_variant_new_string("Playing");

    else
        return g_variant_new_string("Paused");
}

static void send_properties_changed_signal(GVariant* changed_properties)
{
    if (connection == NULL)
    {
        // D-Bus connection hasn't started yet, so the new entry will
        // simply be queried above instead.
        return;
    }

    // There aren't any invalidated properties
    GVariantBuilder invalidated_builder;
    g_variant_builder_init(&invalidated_builder, G_VARIANT_TYPE("as"));
    GVariant* invalidated_properties = g_variant_builder_end(&invalidated_builder);

    // Final properties variant
    GVariant* parameters = g_variant_new(
        "(s@a{sv}@as)",
        "org.mpris.MediaPlayer2.Player",
        changed_properties,
        invalidated_properties
    );

    g_dbus_connection_emit_signal(
        connection,
        NULL,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        parameters,
        NULL
    );
}

void dbus_set_current_playlist_entry(PlaylistEntry* entry)
{
    current_entry = entry;
    GVariant* metadata = get_metadata_for_current_entry();

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&builder, "{sv}", "Metadata", metadata);
    send_properties_changed_signal(g_variant_builder_end(&builder));
}

void dbus_on_playback_toggled()
{
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(
        &builder,
        "{sv}",
        "PlaybackStatus",
        get_playback_status_variant()
    );
    send_properties_changed_signal(g_variant_builder_end(&builder));
}

void close_dbus()
{
    g_bus_unown_name(owner_id);
}
