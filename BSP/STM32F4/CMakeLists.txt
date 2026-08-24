# Self-contained BSP build description. Exactly one BSP directory is added to
# a configured tree, so all names in this file intentionally stay generic.
set(BSP_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(BSP_LINKER_SCRIPT "${BSP_ROOT}/STM32F407xx_FLASH.ld")

# CPU/FPU flags and the RT-Thread Cortex-M4 port are board-owned configuration.
include("${BSP_ROOT}/Ports/rtthread_cpu_port.cmake")

add_library(bsp OBJECT
    # Startup
    "${BSP_ROOT}/startup_stm32f407xx.s"

    # STM32 HAL drivers
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c"

    # STM32 core and peripheral initialization
    "${BSP_ROOT}/Core/Src/gpio.c"
    "${BSP_ROOT}/Core/Src/rtc.c"
    "${BSP_ROOT}/Core/Src/stm32f4xx_hal_msp.c"
    "${BSP_ROOT}/Core/Src/stm32f4xx_it.c"
    "${BSP_ROOT}/Core/Src/system_stm32f4xx.c"
    "${BSP_ROOT}/Core/Src/usart.c"

    # RT-Thread board ports
    "${BSP_ROOT}/Ports/board.c"
    "${BSP_ROOT}/Ports/drv_rtc.c"
    "${BSP_ROOT}/Ports/drv_usart.c")

# STM32F407 is a single-bank device.  ST's shared F4 HAL keeps the Banks
# parameter for dual-bank parts, so suppress this vendor-only warning without
# weakening diagnostics for BSP or application sources.
set_source_files_properties(
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c"
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")

target_link_libraries(bsp PRIVATE rtos_compile_settings)
target_include_directories(bsp PRIVATE
    # STM32 HAL and CMSIS headers
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc"
    "${BSP_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "${BSP_ROOT}/Drivers/CMSIS/Include"
    "${BSP_ROOT}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"

    # STM32 core configuration
    "${BSP_ROOT}/Core/Inc"

    # RT-Thread board ports
    "${BSP_ROOT}/Ports")
target_compile_definitions(bsp PRIVATE STM32F407xx USE_HAL_DRIVER)
target_compile_options(bsp PRIVATE ${BSP_CPU_COMPILE_OPTIONS})
set_target_properties(bsp PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)

# Publish this BSP's build contract to the BSP registry.
set(BSP_SELECTED_TARGET bsp PARENT_SCOPE)
set(BSP_SELECTED_LINKER_SCRIPT "${BSP_LINKER_SCRIPT}" PARENT_SCOPE)
set(BSP_SELECTED_CPU_COMPILE_OPTIONS "${BSP_CPU_COMPILE_OPTIONS}" PARENT_SCOPE)
set(BSP_SELECTED_CPU_LINK_OPTIONS "${BSP_CPU_LINK_OPTIONS}" PARENT_SCOPE)
set(BSP_SELECTED_RTTHREAD_PORT_ID "${BSP_SELECTED_RTTHREAD_PORT_ID}" PARENT_SCOPE)
set(BSP_SELECTED_RTTHREAD_PORT_SOURCES "${BSP_SELECTED_RTTHREAD_PORT_SOURCES}" PARENT_SCOPE)
set(BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS "${BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS}" PARENT_SCOPE)
set(BSP_SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS
    "${BSP_SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS}"
    PARENT_SCOPE)
