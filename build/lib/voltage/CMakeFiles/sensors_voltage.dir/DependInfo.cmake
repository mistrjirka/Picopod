
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/usr/share/pico-sdk/src/rp2_common/hardware_divider/divider.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_divider/divider.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/hardware_irq/irq_handler_chain.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_irq/irq_handler_chain.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_bit_ops/bit_ops_aeabi.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_bit_ops/bit_ops_aeabi.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_divider/divider.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_divider/divider.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_double/double_aeabi.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_aeabi.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_double/double_v1_rom_shim.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_v1_rom_shim.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_float/float_aeabi.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_aeabi.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_float/float_v1_rom_shim.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_v1_rom_shim.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_int64_ops/pico_int64_ops_aeabi.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_int64_ops/pico_int64_ops_aeabi.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_mem_ops/mem_ops_aeabi.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_mem_ops/mem_ops_aeabi.S.obj"
  "/usr/share/pico-sdk/src/rp2_common/pico_standard_link/crt0.S" "/home/jirka/programovani/pico/Picopod/build/lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_standard_link/crt0.S.obj"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "LIB_PICO_BIT_OPS=1"
  "LIB_PICO_BIT_OPS_PICO=1"
  "LIB_PICO_DIVIDER=1"
  "LIB_PICO_DIVIDER_HARDWARE=1"
  "LIB_PICO_DOUBLE=1"
  "LIB_PICO_DOUBLE_PICO=1"
  "LIB_PICO_FLOAT=1"
  "LIB_PICO_FLOAT_PICO=1"
  "LIB_PICO_INT64_OPS=1"
  "LIB_PICO_INT64_OPS_PICO=1"
  "LIB_PICO_MALLOC=1"
  "LIB_PICO_MEM_OPS=1"
  "LIB_PICO_MEM_OPS_PICO=1"
  "LIB_PICO_PLATFORM=1"
  "LIB_PICO_PRINTF=1"
  "LIB_PICO_PRINTF_PICO=1"
  "LIB_PICO_RUNTIME=1"
  "LIB_PICO_STANDARD_LINK=1"
  "LIB_PICO_STDIO=1"
  "LIB_PICO_STDIO_UART=1"
  "LIB_PICO_STDLIB=1"
  "LIB_PICO_SYNC=1"
  "LIB_PICO_SYNC_CORE=1"
  "LIB_PICO_SYNC_CRITICAL_SECTION=1"
  "LIB_PICO_SYNC_MUTEX=1"
  "LIB_PICO_SYNC_SEM=1"
  "LIB_PICO_TIME=1"
  "LIB_PICO_UTIL=1"
  "PICO_BOARD=\"pico\""
  "PICO_BUILD=1"
  "PICO_CMAKE_BUILD_TYPE=\"Release\""
  "PICO_COPY_TO_RAM=0"
  "PICO_CXX_ENABLE_EXCEPTIONS=0"
  "PICO_NO_FLASH=0"
  "PICO_NO_HARDWARE=0"
  "PICO_ON_DEVICE=1"
  "PICO_USE_BLOCKED_RAM=0"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "/usr/share/pico-sdk/src/common/pico_stdlib/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_gpio/include"
  "/usr/share/pico-sdk/src/common/pico_base/include"
  "generated/pico_base"
  "/usr/share/pico-sdk/src/boards/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_platform/include"
  "/usr/share/pico-sdk/src/rp2040/hardware_regs/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_base/include"
  "/usr/share/pico-sdk/src/rp2040/hardware_structs/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_claim/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_sync/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_irq/include"
  "/usr/share/pico-sdk/src/common/pico_sync/include"
  "/usr/share/pico-sdk/src/common/pico_time/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_timer/include"
  "/usr/share/pico-sdk/src/common/pico_util/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_uart/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_divider/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_runtime/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_clocks/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_resets/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_pll/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_vreg/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_watchdog/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_xosc/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_printf/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_bootrom/include"
  "/usr/share/pico-sdk/src/common/pico_bit_ops/include"
  "/usr/share/pico-sdk/src/common/pico_divider/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_double/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_int64_ops/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_float/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_malloc/include"
  "/usr/share/pico-sdk/src/rp2_common/boot_stage2/include"
  "/usr/share/pico-sdk/src/common/pico_binary_info/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_stdio/include"
  "/usr/share/pico-sdk/src/rp2_common/pico_stdio_uart/include"
  "/usr/share/pico-sdk/src/rp2_common/hardware_adc/include"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/usr/share/pico-sdk/src/common/pico_sync/critical_section.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/critical_section.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/critical_section.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_sync/lock_core.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/lock_core.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/lock_core.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_sync/mutex.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/mutex.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/mutex.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_sync/sem.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/sem.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_sync/sem.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_time/time.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_time/time.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_time/time.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_time/timeout_helper.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_time/timeout_helper.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_time/timeout_helper.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_util/datetime.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/datetime.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/datetime.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_util/pheap.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/pheap.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/pheap.c.obj.d"
  "/usr/share/pico-sdk/src/common/pico_util/queue.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/queue.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/common/pico_util/queue.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_adc/adc.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_adc/adc.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_adc/adc.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_claim/claim.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_claim/claim.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_claim/claim.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_clocks/clocks.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_clocks/clocks.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_clocks/clocks.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_gpio/gpio.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_gpio/gpio.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_gpio/gpio.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_irq/irq.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_irq/irq.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_irq/irq.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_pll/pll.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_pll/pll.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_pll/pll.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_sync/sync.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_sync/sync.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_sync/sync.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_timer/timer.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_timer/timer.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_timer/timer.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_uart/uart.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_uart/uart.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_uart/uart.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_vreg/vreg.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_vreg/vreg.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_vreg/vreg.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_watchdog/watchdog.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_watchdog/watchdog.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_watchdog/watchdog.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/hardware_xosc/xosc.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_xosc/xosc.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/hardware_xosc/xosc.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_bootrom/bootrom.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_bootrom/bootrom.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_bootrom/bootrom.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_double/double_init_rom.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_init_rom.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_init_rom.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_double/double_math.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_math.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_double/double_math.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_float/float_init_rom.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_init_rom.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_init_rom.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_float/float_math.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_math.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_float/float_math.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_platform/platform.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_platform/platform.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_platform/platform.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_printf/printf.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_printf/printf.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_printf/printf.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_runtime/runtime.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_runtime/runtime.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_runtime/runtime.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_standard_link/binary_info.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_standard_link/binary_info.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_standard_link/binary_info.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_stdio/stdio.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdio/stdio.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdio/stdio.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_stdio_uart/stdio_uart.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdio_uart/stdio_uart.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdio_uart/stdio_uart.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_stdlib/stdlib.c" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdlib/stdlib.c.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_stdlib/stdlib.c.obj.d"
  "/usr/share/pico-sdk/src/rp2_common/pico_standard_link/new_delete.cpp" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_standard_link/new_delete.cpp.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/usr/share/pico-sdk/src/rp2_common/pico_standard_link/new_delete.cpp.obj.d"
  "/home/jirka/programovani/pico/Picopod/lib/voltage/voltage.cpp" "lib/voltage/CMakeFiles/sensors_voltage.dir/voltage.cpp.obj" "gcc" "lib/voltage/CMakeFiles/sensors_voltage.dir/voltage.cpp.obj.d"
  )

# Targets to which this target links.
set(CMAKE_TARGET_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
