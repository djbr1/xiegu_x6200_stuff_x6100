cmake_minimum_required(VERSION 3.10)
project(build LANGUAGES C)

set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{HOME}/x6200/AetherX6200Buildroot/build/host/arm-buildroot-linux-gnueabihf/sysroot")
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_SYSROOT_DIR}/usr/lib/pkgconfig:$ENV{PKG_CONFIG_SYSROOT_DIR}/usr/share/pkgconfig")
set(CMAKE_SYSROOT "$ENV{PKG_CONFIG_SYSROOT_DIR}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.0 gstreamer-app-1.0>=1.0)
add_executable(build main.c)

target_include_directories(build PRIVATE ${GST_INCLUDE_DIRS})
target_link_libraries(build ${GST_LIBRARIES} m -pthread)

message(STATUS "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
message(STATUS "GStreamer libraries: ${GST_LIBRARIES}")
