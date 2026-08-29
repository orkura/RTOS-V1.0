# SEGGER RTT is currently enabled only for the STM32H743VIT BSP.  Keeping the
# selection here prevents the shared component registry from changing existing
# STM32F4 and CC2538 images.
if(NOT TARGET_BSP STREQUAL "stm32h743vit")
    return()
endif()

set(SEGGER_RTT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

add_library(segger_rtt_component OBJECT
    "${SEGGER_RTT_ROOT}/RTT/SEGGER_RTT.c"
    "${SEGGER_RTT_ROOT}/RTT/SEGGER_RTT_printf.c"
    "${SEGGER_RTT_ROOT}/RTT/SEGGER_RTT_ASM_ARMv7M.S"
    "${SEGGER_RTT_ROOT}/adapter/drv_rtt.c")

target_link_libraries(segger_rtt_component PRIVATE rtos_compile_settings)
target_include_directories(segger_rtt_component PRIVATE
    "${SEGGER_RTT_ROOT}/RTT")
set_target_properties(segger_rtt_component PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)
