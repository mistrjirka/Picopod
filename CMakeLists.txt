# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(Picopod C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# Pull in Raspberry Pi Pico SDK (must be before project)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(Picopod Picopod.h Picopod.cpp)

pico_set_program_name(Picopod "Picopod")
pico_set_program_version(Picopod "0.1")
add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

pico_enable_stdio_usb(Picopod 1)
pico_enable_stdio_uart(Picopod 0)
add_subdirectory(lib)
target_link_libraries(Picopod pico_stdlib)

# Add any user requested libraries
target_link_libraries(Picopod
        hardware_spi
        hardware_timer
        hardware_watchdog
        hardware_interp
        hardware_flash
        hardware_i2c
        hardware_uart
        sensors_voltage
        mathextension
        mac
        lcmm
        DTP
        IOUSBBT
)


pico_add_extra_outputs(Picopod)

