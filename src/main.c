#include <adwaita.h>
#include <glib/gstdio.h>

static void on_click(GtkButton* button)
{
    (void)button;
    printf("Hello world!\n");
}

static void on_activate(GtkApplication *app)
{
    GtkBuilder* builder = gtk_builder_new_from_resource("/com/github/lukawarren/waveform/src/ui/window.ui");

    GObject* window = gtk_builder_get_object(builder, "window");
    gtk_window_set_application(GTK_WINDOW(window), app);

    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
    g_object_unref(builder);
}

int main(int argc, char** argv)
{
    g_autoptr(AdwApplication) app = NULL;
    app = adw_application_new("com.github.lukawarren.waveform", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}