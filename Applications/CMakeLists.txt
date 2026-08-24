# Applications are fixed for this project.  This file deliberately provides no
# application-selection cache variable or profile mechanism.
add_library(application OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/main.c")

target_link_libraries(application PRIVATE rtos_compile_settings)
set_target_properties(application PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)
