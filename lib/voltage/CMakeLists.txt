add_library(sensors_voltage voltage.cpp voltage.h)

target_link_libraries(sensors_voltage
    pico_stdlib 
    hardware_adc
    hardware_gpio
    hardware_timer
)
