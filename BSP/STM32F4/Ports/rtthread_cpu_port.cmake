# STM32F407ZGT6 CPU and RT-Thread architecture-port contract.
#
# The selected device contains a Cortex-M4F with an FPv4-SP-D16 FPU.  This BSP
# uses the hard-float ABI consistently for BSP, kernel, application and link
# steps.  RT-Thread's GNU Cortex-M4 context switch implementation is reused
# directly from libcpu; the board owns the choice and validates every path.

set(BSP_CPU_COMPILE_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)
set(BSP_CPU_LINK_OPTIONS ${BSP_CPU_COMPILE_OPTIONS})

set(BSP_SELECTED_RTTHREAD_PORT_ID "arm/cortex-m4")
set(_rtthread_cpu_port_root
    "${PROJECT_SOURCE_DIR}/RT-Thread/libcpu/${BSP_SELECTED_RTTHREAD_PORT_ID}")
set(BSP_SELECTED_RTTHREAD_PORT_SOURCES
    "${_rtthread_cpu_port_root}/cpuport.c"
    "${_rtthread_cpu_port_root}/context_gcc.S")
set(BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS
    "${_rtthread_cpu_port_root}")
set(BSP_SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS "")

foreach(_cpu_port_source IN LISTS BSP_SELECTED_RTTHREAD_PORT_SOURCES)
    if(NOT EXISTS "${_cpu_port_source}")
        message(FATAL_ERROR
            "STM32F4 RT-Thread CPU port source does not exist: ${_cpu_port_source}")
    endif()
endforeach()

foreach(_cpu_port_include_dir IN LISTS BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS)
    if(NOT IS_DIRECTORY "${_cpu_port_include_dir}")
        message(FATAL_ERROR
            "STM32F4 RT-Thread CPU port include directory does not exist: "
            "${_cpu_port_include_dir}")
    endif()
endforeach()

unset(_cpu_port_source)
unset(_cpu_port_include_dir)
unset(_rtthread_cpu_port_root)
