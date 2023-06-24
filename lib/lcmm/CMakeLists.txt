add_library(lcmm lcmm.cpp lcmm.h)

target_link_libraries(loramessenger
    pico_stdlib
    LoRa_pico_lib
    mathextension
    bluetooth
    hardware_timer
)
