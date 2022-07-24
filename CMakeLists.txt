# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# Pull in Raspberry Pi Pico SDK (must be before project)
set(PICO_SDK_PATH "/home/jirka/programovani/pico/pico-sdk")
project(Picopod C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(Picopod Picopod.cpp )

pico_set_program_name(Picopod "Picopod")
pico_set_program_version(Picopod "0.1")


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
        LoRa_pico_lib
        sensors_voltage
)


pico_add_extra_outputs(Picopod)

