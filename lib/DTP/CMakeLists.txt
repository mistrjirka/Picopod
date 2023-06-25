add_library(DTP DTP.cpp DTP.h generalsettings.h)

target_link_libraries(DTP
    pico_stdlib
    LoRa_pico_lib
    mathextension
    bluetooth
    hardware_timer
)
