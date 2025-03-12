#include "my_application.h"
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <flutter_linux/flutter_linux.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"

struct _MyApplication {
  GtkApplication parent_instance;
  char** dart_entrypoint_arguments;
  FlMethodChannel* channel;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)


static void gtk_shell_init(GtkWindow* window) {
  gtk_layer_init_for_window(window);
  gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
  gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
  gtk_layer_auto_exclusive_zone_enable (window);
  
  static constexpr gboolean anchors[] = {TRUE, TRUE, TRUE, FALSE};
  for (int i = 0; i < GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER; i++) {
      gtk_layer_set_anchor (window, static_cast<GtkLayerShellEdge>(i), anchors[i]);
  }
}

static void transparency(GtkWindow* window, FlView* view) {
  gtk_widget_set_app_paintable(GTK_WIDGET(window), TRUE);

  GdkScreen* screen = gtk_widget_get_screen(GTK_WIDGET(window));
  GdkVisual* visual = gdk_screen_get_rgba_visual(screen);

  if (visual == nullptr) return;

  gtk_widget_set_visual(GTK_WIDGET(window), visual);

  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(cssProvider, "window { background-color: rgba(0, 0, 0, 0.0); }", -1, NULL);
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(cssProvider);

  GdkRGBA background_color;
  gdk_rgba_parse(&background_color, "#00000000");
  fl_view_set_background_color(view, &background_color);
}

static void method_call_handler(FlMethodChannel* channel,FlMethodCall* method_call,gpointer user_data) {
  // g_autoptr(FlMethodResponse) response = nullptr;


  g_autoptr(GError) error = nullptr;
  g_warning("method_call: %s", fl_method_call_get_name(method_call));

  if (!fl_method_call_respond_success(method_call, nullptr, &error)) {
    g_warning("Failed to send response: %s", error->message);
  }

}

static void register_flutter_channel(MyApplication *self, FlView *view) {

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  self->channel = fl_method_channel_new(
      fl_engine_get_binary_messenger(fl_view_get_engine(view)),
      "internal.window.manager", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      self->channel, method_call_handler, self, nullptr);
}



// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  GtkWindow* window =
      GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));


  GdkRectangle workarea = {0};
  GdkMonitor* monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
  gdk_monitor_get_workarea(monitor,&workarea);

  gtk_widget_set_size_request(GTK_WIDGET(window), workarea.width, 24);
  gtk_shell_init(window); 


  g_autoptr(FlDartProject) project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);

  FlView* view = fl_view_new(project);
  transparency(window, view);

  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  gtk_widget_show_all (GTK_WIDGET (window));
  gtk_layer_try_force_commit(window);

  fl_register_plugins(FL_PLUGIN_REGISTRY(view));
  register_flutter_channel(self, view);
  gtk_widget_grab_focus(GTK_WIDGET(view));
}


// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
  MyApplication* self = MY_APPLICATION(application);
  // Strip out the first argument as it is the binary name.
  self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

  g_autoptr(GError) error = nullptr;
  if (!g_application_register(application, nullptr, &error)) {
     g_warning("Failed to register: %s", error->message);
     *exit_status = 1;
     return TRUE;
  }

  g_application_activate(application);
  *exit_status = 0;

  return TRUE;
}

// Implements GApplication::startup.
static void my_application_startup(GApplication* application) {
  //MyApplication* self = MY_APPLICATION(object);

  // Perform any actions required at application startup.

  G_APPLICATION_CLASS(my_application_parent_class)->startup(application);
}

// Implements GApplication::shutdown.
static void my_application_shutdown(GApplication* application) {
  //MyApplication* self = MY_APPLICATION(object);

  // Perform any actions required at application shutdown.

  G_APPLICATION_CLASS(my_application_parent_class)->shutdown(application);
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object) {
  MyApplication* self = MY_APPLICATION(object);
  g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
  g_clear_object(&self->channel);
  G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

static void my_application_class_init(MyApplicationClass* klass) {
  G_APPLICATION_CLASS(klass)->activate = my_application_activate;
  G_APPLICATION_CLASS(klass)->local_command_line = my_application_local_command_line;
  G_APPLICATION_CLASS(klass)->startup = my_application_startup;
  G_APPLICATION_CLASS(klass)->shutdown = my_application_shutdown;
  G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
}

static void my_application_init(MyApplication* self) {}

MyApplication* my_application_new() {
  // Set the program name to the application ID, which helps various systems
  // like GTK and desktop environments map this running application to its
  // corresponding .desktop file. This ensures better integration by allowing
  // the application to be recognized beyond its binary name.
  g_set_prgname(APPLICATION_ID);

  return MY_APPLICATION(g_object_new(my_application_get_type(),
                                     "application-id", APPLICATION_ID,
                                     "flags", G_APPLICATION_NON_UNIQUE,
                                     nullptr));
}
