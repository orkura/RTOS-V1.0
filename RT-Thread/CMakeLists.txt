# RT-Thread sources controlled by the selected BSP's rtconfig.h.
# The BSP registry must select a CPU port before this directory is added.  This
# keeps the kernel independent of board names while allowing M3/M4/M7/etc.
# ports to be selected by the BSP.
foreach(_required_port_value IN ITEMS
        SELECTED_RTTHREAD_PORT_ID
        SELECTED_RTTHREAD_PORT_SOURCES
        SELECTED_RTTHREAD_PORT_INCLUDE_DIRS)
    if(NOT DEFINED ${_required_port_value} OR "${${_required_port_value}}" STREQUAL "")
        message(FATAL_ERROR
            "RT-Thread CPU port was not selected: missing '${_required_port_value}'.")
    endif()
endforeach()

# Keep this target as an OBJECT library: component-init and FinSH table objects
# must reach the final linker script unchanged.
add_library(rtthread_kernel OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/src/clock.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/components.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpu.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/device.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/idle.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/ipc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/irq.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/kservice.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/mem.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/object.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/scheduler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/slab.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/thread.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/timer.c"
    "${CMAKE_CURRENT_LIST_DIR}/components/finsh/cmd.c"
    "${CMAKE_CURRENT_LIST_DIR}/components/finsh/msh.c"
    "${CMAKE_CURRENT_LIST_DIR}/components/finsh/msh_parse.c"
    "${CMAKE_CURRENT_LIST_DIR}/components/finsh/shell.c"
    ${SELECTED_RTTHREAD_PORT_SOURCES})

target_link_libraries(rtthread_kernel PRIVATE rtos_build_config)
target_include_directories(rtthread_kernel PRIVATE
    ${SELECTED_RTTHREAD_PORT_INCLUDE_DIRS}
    ${SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS})
target_compile_options(rtthread_kernel PRIVATE
    -Wno-unused-parameter
    -Wno-implicit-fallthrough
    -Wno-cast-function-type)
set_target_properties(rtthread_kernel PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)
