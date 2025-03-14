cmake_minimum_required(VERSION 3.13)
project(runner LANGUAGES CXX)


pkg_check_modules(GTK_LAYER_SHELL REQUIRED gtk-layer-shell-0)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(GTK_WAYLAND REQUIRED gdk-wayland-3.0)

# Define the application target. To change its name, change BINARY_NAME in the
# top-level CMakeLists.txt, not the value here, or `flutter run` will no longer
# work.
#
# Any new source files that you add to the application should be added here.
add_executable(${BINARY_NAME}
  "main.cc"
  "my_application.cc"
  "${FLUTTER_MANAGED_DIR}/generated_plugin_registrant.cc"
  "simdjson.h"
  "simdjson.cpp"
)

# Apply the standard set of build settings. This can be removed for applications
# that need different build settings.
apply_standard_settings(${BINARY_NAME})

# Add preprocessor definitions for the application ID.
add_definitions(-DAPPLICATION_ID="${APPLICATION_ID}")

# Add dependency libraries. Add any application-specific dependencies here.
target_link_libraries(${BINARY_NAME} PRIVATE flutter)
target_link_libraries(${BINARY_NAME} PRIVATE PkgConfig::GTK)
target_link_libraries(${BINARY_NAME} PRIVATE ${GTK_LAYER_SHELL_LIBRARIES})
target_link_libraries(${BINARY_NAME} PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
target_link_libraries(${BINARY_NAME} PRIVATE ${GTK_WAYLAND_LIBRARIES})

target_include_directories(${BINARY_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(${BINARY_NAME} PRIVATE ${GTK_WAYLAND_INCLUDE_DIRS})
target_include_directories(${BINARY_NAME} PRIVATE ${GTK_LAYER_SHELL_INCLUDE_DIRS})


target_compile_options(${BINARY_NAME} PRIVATE ${GTK_LAYER_SHELL_CFLAGS_OTHER})
target_compile_options(${BINARY_NAME} PRIVATE ${WAYLAND_CLIENT_CFLAGS_OTHER})
target_compile_options(${BINARY_NAME} PRIVATE ${GTK_WAYLAND_CFLAGS_OTHER})