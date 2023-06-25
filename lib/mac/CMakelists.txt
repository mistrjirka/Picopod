add_library(mac mac.cpp mac.h)

target_link_libraries(mac
    pico_stdlib
    LoRa_pico_lib
    mathextension
    bluetooth
    hardware_timer
)
