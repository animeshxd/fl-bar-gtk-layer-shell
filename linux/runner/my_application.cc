#include "my_application.h"
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <optional>
#include <flutter_linux/flutter_linux.h>
#include <gdk/gdkwayland.h>

#include "flutter/generated_plugin_registrant.h"
#include "simdjson.h"

// Add this function declaration at the top of the file
static gboolean on_window_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

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
  gtk_css_provider_load_from_data(cssProvider, "window { background-color: rgba(0, 0, 0, 0.0); }", -1, nullptr);
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(cssProvider);

  GdkRGBA background_color;
  gdk_rgba_parse(&background_color, "#00000000");
  fl_view_set_background_color(view, &background_color);
}

struct Margin {
    double left = 0;
    double top = 0;
    double right = 0;
    double bottom = 0;
};

enum Position { TOP_LEFT, TOP_CENTER, TOP_RIGHT };

struct Params {
    int width;
    int height;
    Position position;
    Margin margin;

    Params(const int width, const int height) {
        this->width = width;
        this->height = height;
        position = TOP_LEFT;
        margin.left = margin.top = margin.right = margin.bottom = 0;
    }

    Params() : width(0), height(0), position(TOP_CENTER) {}

    static std::optional<Params> fromJSON(const gchar* jsons) {
        simdjson::dom::parser parser;
        simdjson::dom::element doc;
        auto error = parser.parse(jsons, std::strlen(jsons)).get(doc);
        if (error) {
            g_critical("Error parsing JSON: %s",
                       simdjson::error_message(error));
            return std::nullopt;
        }

        Params params;
        int64_t width, height, pos;
        double margin_left, margin_top, margin_right, margin_bottom;

        if (doc["width"].get(width) || doc["height"].get(height) ||
            doc["position"].get(pos) ||
            doc["margin"]["left"].get(margin_left) ||
            doc["margin"]["top"].get(margin_top) ||
            doc["margin"]["right"].get(margin_right) ||
            doc["margin"]["bottom"].get(margin_bottom)) {
            g_critical("Error parsing JSON");
            return std::nullopt;
        }

        params.width = width;
        params.height = height;

        if (pos == 0) {
            params.position = TOP_LEFT;
        } else if (pos == 1) {
            params.position = TOP_CENTER;
        } else if (pos == 2) {
            params.position = TOP_RIGHT;
        }

        params.margin.left = margin_left;
        params.margin.top = margin_top;
        params.margin.right = margin_right;
        params.margin.bottom = margin_bottom;

        g_warning(
            "Params: width=%d, height=%d, position=%d, margin.left=%f, "
            "margin.top=%f, margin.right=%f, margin.bottom=%f",
            params.width, params.height, params.position, params.margin.left,
            params.margin.top, params.margin.right, params.margin.bottom);
        return params;
    }
};

static void gtk_shell_init(GtkWindow* window, const Params& params) {
    gtk_layer_init_for_window(window);

    gtk_layer_set_keyboard_mode(
        window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);  // only handled focus
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_auto_exclusive_zone_enable(window);

    gboolean anchors[] = {TRUE, TRUE, TRUE, TRUE};

    anchors[GTK_LAYER_SHELL_EDGE_BOTTOM] = FALSE;

    switch (params.position) {
        case TOP_CENTER:
            anchors[GTK_LAYER_SHELL_EDGE_BOTTOM] = FALSE;
            break;
        case TOP_LEFT:
            anchors[GTK_LAYER_SHELL_EDGE_RIGHT] = FALSE;
            break;
        case TOP_RIGHT:
            anchors[GTK_LAYER_SHELL_EDGE_LEFT] = FALSE;
            break;
    }

    for (int i = 0; i < GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER; i++) {
        gtk_layer_set_anchor(window, static_cast<GtkLayerShellEdge>(i),
                             anchors[i]);
    }
}

// Add the function implementation before create_new_Window
static void on_window_realize(GtkWidget *widget, gpointer data) {
    // GtkWindow *window = GTK_WINDOW(widget);
    
    // Get display and monitor safely for Wayland
    GdkWindow *gdk_window = gtk_widget_get_window(widget);
    if (!gdk_window) {
        g_warning("No GDK window available");
        return;
    }

    GdkDisplay *display = gdk_window_get_display(gdk_window);
    if (!display) {
        g_warning("No display available");
        return;
    }

    // Get the monitor where the window is located
    GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gdk_window);
    if (!monitor) {
        g_warning("No monitor found for window");
        return;
    }

    // Get monitor geometry
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    g_warning("Monitor geometry: %dx%d at (%d,%d)", 
              geometry.width, geometry.height,
              geometry.x, geometry.y);

    gtk_widget_set_size_request(widget, geometry.width, geometry.height);
}

// Modify the create_new_Window function
static void create_new_Window(const Params& params) {
    GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    g_autoptr(FlDartProject) project = fl_dart_project_new();
    const char* entrypoint_args[] = {"ignore", nullptr};
    fl_dart_project_set_dart_entrypoint_arguments(project, const_cast<char**>(entrypoint_args));

    // Set window size to 700x700
    // gtk_widget_set_size_request(GTK_WIDGET(window),700, 700);

    gtk_shell_init(window, params);
    g_signal_connect(window, "realize", G_CALLBACK(on_window_realize), nullptr);

    FlView* view = fl_view_new(project);
    gtk_widget_set_size_request(GTK_WIDGET(view), params.width, params.height);
    
    // Create a container to center the view
    GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_box, GTK_ALIGN_START);
    
    // Add view to the center box
    gtk_box_pack_start(GTK_BOX(center_box), GTK_WIDGET(view), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), center_box);
    
    // Add click event handler
    g_signal_connect(GTK_WIDGET(window), "button-press-event", 
                    G_CALLBACK(on_window_clicked), view);

    transparency(window, view);
    
    gtk_widget_show_all(GTK_WIDGET(window));
}

// Add this function implementation
static gboolean on_window_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    FlView *view = FL_VIEW(user_data);
    GtkAllocation view_allocation;
    gtk_widget_get_allocation(GTK_WIDGET(view), &view_allocation);
    
    // Convert coordinates relative to the window
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(view));
    gint wx, wy;
    gtk_widget_translate_coordinates(GTK_WIDGET(view), window, 0, 0, &wx, &wy);
    
    // Check if click is outside the view area
    if (event->x < wx || 
        event->x > (wx + view_allocation.width) ||
        event->y < wy || 
        event->y > (wy + view_allocation.height)) {
        g_warning("Clicked outside view at x: %f, y: %f", event->x, event->y);
        gtk_widget_destroy(widget);
        return TRUE; // Handle the event
    }
    return FALSE; // Let the event propagate
}

static void method_call_handler(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
    // g_autoptr(FlMethodResponse) response = nullptr;
    const auto method_name = fl_method_call_get_name(method_call);
    auto arg = fl_method_call_get_args(method_call);
    const auto value = fl_value_get_string(arg);

    if (fl_value_get_type(arg) != FL_VALUE_TYPE_STRING) {
        g_critical("The argument is not a string");
    }

    if (strcmp(method_name, "new_window") == 0) {
        g_warning("method_call: %s value %s", method_name, value);
        auto params = Params::fromJSON(value);
        if (params.has_value()) create_new_Window(params.value());
    }
    g_autoptr(GError) error = nullptr;
    g_warning("method_call: %s ", method_name);

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
