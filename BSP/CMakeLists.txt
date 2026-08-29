# This is the only target-selection registry.  Exactly one BSP is added to a
# configured build tree so startup vectors, linker scripts and board drivers
# can never be mixed.
if(TARGET_BSP STREQUAL "cc2538")
    add_subdirectory(CC2538)
elseif(TARGET_BSP STREQUAL "stm32f4")
    add_subdirectory(STM32F4)
elseif(TARGET_BSP STREQUAL "stm32h743vit")
    add_subdirectory(STM32H743VIT)
else()
    message(FATAL_ERROR
        "Unsupported TARGET_BSP='${TARGET_BSP}'. Supported values: "
        "cc2538, stm32f4, stm32h743vit.")

endif()

if(NOT DEFINED BSP_SELECTED_TARGET)
    message(FATAL_ERROR "Selected BSP did not publish its object target.")
endif()

foreach(_required_bsp_value IN ITEMS
        BSP_SELECTED_LINKER_SCRIPT
        BSP_SELECTED_CPU_COMPILE_OPTIONS
        BSP_SELECTED_CPU_LINK_OPTIONS
        BSP_SELECTED_RTTHREAD_PORT_ID
        BSP_SELECTED_RTTHREAD_PORT_SOURCES
        BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS)
    if(NOT DEFINED ${_required_bsp_value} OR "${${_required_bsp_value}}" STREQUAL "")
        message(FATAL_ERROR
            "Selected BSP did not publish required platform value '${_required_bsp_value}'.")
    endif()
endforeach()

# Publish the selected BSP contract to the top-level build.
set(SELECTED_BSP_TARGET "${BSP_SELECTED_TARGET}" PARENT_SCOPE)
set(SELECTED_BSP_LINKER_SCRIPT "${BSP_SELECTED_LINKER_SCRIPT}" PARENT_SCOPE)
set(SELECTED_BSP_CPU_COMPILE_OPTIONS "${BSP_SELECTED_CPU_COMPILE_OPTIONS}" PARENT_SCOPE)
set(SELECTED_BSP_CPU_LINK_OPTIONS "${BSP_SELECTED_CPU_LINK_OPTIONS}" PARENT_SCOPE)
set(SELECTED_RTTHREAD_PORT_ID "${BSP_SELECTED_RTTHREAD_PORT_ID}" PARENT_SCOPE)
set(SELECTED_RTTHREAD_PORT_SOURCES "${BSP_SELECTED_RTTHREAD_PORT_SOURCES}" PARENT_SCOPE)
set(SELECTED_RTTHREAD_PORT_INCLUDE_DIRS "${BSP_SELECTED_RTTHREAD_PORT_INCLUDE_DIRS}" PARENT_SCOPE)
set(SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS
    "${BSP_SELECTED_RTTHREAD_PORT_PRIVATE_INCLUDE_DIRS}"
    PARENT_SCOPE)
