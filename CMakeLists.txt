cmake_minimum_required(VERSION 3.20)

# TARGET_BSP is intentionally checked before project() so a missing hardware
# target fails before CMake performs a cross-compiler probe.
set(TARGET_BSP "" CACHE STRING
    "Hardware BSP target: cc2538 or stm32f4")
set_property(CACHE TARGET_BSP PROPERTY STRINGS
    cc2538
    stm32f4)

if(TARGET_BSP STREQUAL "")
    message(FATAL_ERROR
        "TARGET_BSP is required. Use -DTARGET_BSP=cc2538 or stm32f4.")
endif()

project(rtos LANGUAGES C ASM)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project currently supports the GNU Arm Embedded Toolchain only.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# RT-Thread feature selection is platform-independent.  The selected BSP owns
# CPU, startup and driver configuration, but never this shared configuration.
set(RTTHREAD_CONFIG_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/rtconfig.h")
if(NOT EXISTS "${RTTHREAD_CONFIG_HEADER}")
    message(FATAL_ERROR "RT-Thread configuration is missing: ${RTTHREAD_CONFIG_HEADER}")
endif()

# Common configuration intentionally excludes BSP-private include paths.
add_library(rtos_compile_settings INTERFACE)
target_include_directories(rtos_compile_settings INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/RT-Thread/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/RT-Thread/components/finsh")
target_compile_features(rtos_compile_settings INTERFACE c_std_11)
target_compile_options(rtos_compile_settings INTERFACE
    # Debug information
    -g3

    # Section-level dead-code elimination
    -ffunction-sections
    -fdata-sections

    # C language behavior
    -fno-common

    # Diagnostics
    -Wall
    -Wextra)

add_subdirectory(BSP)

if(NOT DEFINED SELECTED_BSP_TARGET OR NOT TARGET "${SELECTED_BSP_TARGET}")
    message(FATAL_ERROR "BSP registry did not provide a selected object target.")
endif()

foreach(_required_bsp_value IN ITEMS
        SELECTED_BSP_LINKER_SCRIPT
        SELECTED_BSP_CPU_COMPILE_OPTIONS
        SELECTED_BSP_CPU_LINK_OPTIONS
        SELECTED_RTTHREAD_PORT_ID
        SELECTED_RTTHREAD_PORT_SOURCES
        SELECTED_RTTHREAD_PORT_INCLUDE_DIRS)
    if(NOT DEFINED ${_required_bsp_value} OR "${${_required_bsp_value}}" STREQUAL "")
        message(FATAL_ERROR
            "BSP registry did not provide required platform value '${_required_bsp_value}'.")
    endif()
endforeach()

# Publish the shared RT-Thread configuration without exposing BSP-private
# Ports directories. configure_file also makes CMake regenerate when it changes.
set(RTTHREAD_CONFIG_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/rtconfig")
file(MAKE_DIRECTORY "${RTTHREAD_CONFIG_INCLUDE_DIR}")
configure_file(
    "${RTTHREAD_CONFIG_HEADER}"
    "${RTTHREAD_CONFIG_INCLUDE_DIR}/rtconfig.h"
    COPYONLY)
target_include_directories(rtos_compile_settings INTERFACE
    "${RTTHREAD_CONFIG_INCLUDE_DIR}")
target_compile_options(rtos_compile_settings INTERFACE ${SELECTED_BSP_CPU_COMPILE_OPTIONS})

add_subdirectory(RT-Thread)
add_subdirectory(Applications)

add_executable(rtthread
    $<TARGET_OBJECTS:${SELECTED_BSP_TARGET}>
    $<TARGET_OBJECTS:rtthread_kernel>
    $<TARGET_OBJECTS:application>)

set(RTTHREAD_BASENAME "rtthread-${TARGET_BSP}")
set(RTTHREAD_MAP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RTTHREAD_BASENAME}.map")
set_target_properties(rtthread PROPERTIES
    OUTPUT_NAME "${RTTHREAD_BASENAME}"
    SUFFIX ".elf")

target_link_options(rtthread PRIVATE
    # CPU and ABI
    ${SELECTED_BSP_CPU_LINK_OPTIONS}

    # C runtime
    --specs=nano.specs

    # Memory layout
    "-T${SELECTED_BSP_LINKER_SCRIPT}"

    # Link-time cleanup and reports
    -Wl,--gc-sections
    "-Wl,-Map=${RTTHREAD_MAP_FILE}"
    -Wl,--no-warn-rwx-segments)
target_link_libraries(rtthread PRIVATE rtos_compile_settings m)

get_filename_component(ARM_GNU_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(ARM_NONE_EABI_OBJCOPY
    NAMES arm-none-eabi-objcopy
    HINTS "${ARM_GNU_BIN_DIR}"
    REQUIRED)
find_program(ARM_NONE_EABI_SIZE
    NAMES arm-none-eabi-size
    HINTS "${ARM_GNU_BIN_DIR}"
    REQUIRED)

add_custom_command(TARGET rtthread POST_BUILD
    COMMAND "${ARM_NONE_EABI_OBJCOPY}" -O ihex
            "$<TARGET_FILE:rtthread>"
            "${CMAKE_CURRENT_BINARY_DIR}/${RTTHREAD_BASENAME}.hex"
    COMMAND "${ARM_NONE_EABI_OBJCOPY}" -O binary
            "$<TARGET_FILE:rtthread>"
            "${CMAKE_CURRENT_BINARY_DIR}/${RTTHREAD_BASENAME}.bin"
    COMMAND "${ARM_NONE_EABI_SIZE}" "$<TARGET_FILE:rtthread>"
    BYPRODUCTS
            "${CMAKE_CURRENT_BINARY_DIR}/${RTTHREAD_BASENAME}.hex"
            "${CMAKE_CURRENT_BINARY_DIR}/${RTTHREAD_BASENAME}.bin"
    COMMENT "Generating ${RTTHREAD_BASENAME}.hex and ${RTTHREAD_BASENAME}.bin")
