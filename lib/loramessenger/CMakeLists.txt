add_library(loramessenger loramessenger.cpp loramessenger.h)

target_link_libraries(loramessenger
    pico_stdlib
    LoRa_pico_lib
    mathextension
    bluetooth
)
