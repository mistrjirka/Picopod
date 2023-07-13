add_library(loraairtime LoraAirtime.cpp LoraAirtime.h)

target_link_libraries(loraairtime
    pico_stdlib
    LoRa_pico_lib
)
