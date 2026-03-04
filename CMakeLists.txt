cmake_minimum_required(VERSION 3.10)
project(kibo C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

set(CMAKE_C_FLAGS "-Wall -Werror -Wno-error=unused-const-variable=")
include_directories("include")

add_executable(kibo src/device.c src/exploit.c src/main.c)

target_include_directories(kibo PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(kibo PRIVATE ${LIBUSB_LIBRARIES})
target_compile_options(kibo PRIVATE ${LIBUSB_CFLAGS_OTHER})
