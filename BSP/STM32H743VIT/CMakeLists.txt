# Self-contained STM32H743VIT6 BSP build description.
set(BSP_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(BSP_LINKER_SCRIPT "${BSP_ROOT}/STM32H743xx_FLASH.ld")

include("${BSP_ROOT}/Ports/rtthread_cpu_port.cmake")

add_library(bsp OBJECT
    # Startup
    "${BSP_ROOT}/startup_stm32h743xx.s"

    # STM32 HAL drivers selected by the CubeMX reference project
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c"

    # STM32 core and peripheral initialization
    "${BSP_ROOT}/Core/Src/gpio.c"
    "${BSP_ROOT}/Core/Src/stm32h7xx_hal_msp.c"
    "${BSP_ROOT}/Core/Src/stm32h7xx_it.c"
    "${BSP_ROOT}/Core/Src/system_stm32h7xx.c"
    "${BSP_ROOT}/Core/Src/usart.c"

    # RT-Thread board ports
    "${BSP_ROOT}/Ports/board.c"
    "${BSP_ROOT}/Ports/drv_usart.c")

target_link_libraries(bsp PRIVATE rtos_compile_settings)
target_include_directories(bsp PRIVATE
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc"
    "${BSP_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy"
    "${BSP_ROOT}/Drivers/CMSIS/Include"
    "${BSP_ROOT}/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
    "${BSP_ROOT}/Core/Inc"
    "${BSP_ROOT}/Ports")
target_compile_definitions(bsp PRIVATE
    STM32H743xx
    USE_HAL_DRIVER
    USE_PWR_LDO_SUPPLY)
target_compile_options(bsp PRIVATE ${BSP_CPU_COMPILE_OPTIONS})
set_target_properties(bsp PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)

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
