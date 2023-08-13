add_library(lcmm lcmm.cpp lcmm.h)

target_link_libraries(lcmm
    pico_stdlib
    mathextension
    bluetooth
    hardware_timer
)
