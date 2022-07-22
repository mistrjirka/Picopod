add_library(sensors_voltage STATIC voltage.c)
target_include_directories(sensors_voltage PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(sensors_voltage
    pico_stdlib 
    hardware_adc
    hardware_gpio
    hardware_timer
)
