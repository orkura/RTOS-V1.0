add_library(libc_heap_adapter OBJECT
    "${CMAKE_CURRENT_LIST_DIR}/rtthread_heap_adapter.c")

target_link_libraries(libc_heap_adapter PRIVATE rtos_compile_settings)
set_target_properties(libc_heap_adapter PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES)
