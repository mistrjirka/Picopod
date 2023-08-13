add_library(mac mac.cpp mac.h)

target_link_libraries(mac
    pico_stdlib
    RadioLib
    mathextension
    bluetooth
    hardware_timer
)
