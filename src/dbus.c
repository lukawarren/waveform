#include "dbus.h"
#include <gio/gio.h>
#include <adwaita.h>

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

static const GDBusInterfaceVTable interface_vtable =
{
    on_method_call,
    on_get_property,
    NULL
};

static guint owner_id = 0;

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

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer)
{
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
    printf("bob\n");
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
        if (PROPERTY("PlaybackStatus"))
            return g_variant_new_string("Paued");

        if (PROPERTY("Rate"))
            return g_variant_new_double(1.0);

        if (PROPERTY("MinimumRate"))
            return g_variant_new_double(1.0);

        if (PROPERTY("MaximumRate"))
            return g_variant_new_double(1.0);

        if (PROPERTY("Volume"))
            return g_variant_new_double(1.0);

        if (PROPERTY("Metadata"))
            return NULL;

        if (PROPERTY("Position"))
            return 0;

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

void close_dbus()
{
    g_bus_unown_name(owner_id);
}
