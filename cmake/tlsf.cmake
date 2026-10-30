# Core synchronization stays independent of HAL/FreeRTOS.
if(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS AND NOT RC_FRAMEWORK_ENABLE_TLSF)
    message(FATAL_ERROR "RC_FRAMEWORK_ENABLE_TLSF_FREERTOS requires RC_FRAMEWORK_ENABLE_TLSF=ON")
endif()

# Optional TLSF core, C++ pool, and explicit PMR adapter; no arena, task, or default allocator is created.
if(RC_FRAMEWORK_ENABLE_TLSF)
    set(RC_TLSF_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/TLSF")
    set(RC_TLSF_POOL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Algorithm/memory")
    add_library(rc_framework_tlsf STATIC
        "${RC_TLSF_ROOT}/tlsf.c"
        "${RC_TLSF_POOL_ROOT}/tlsf_pool.cpp"
        "${RC_TLSF_POOL_ROOT}/tlsf_memory_resource.cpp"
        "${RC_TLSF_POOL_ROOT}/tlsf_lock.cpp")
    target_include_directories(rc_framework_tlsf PUBLIC
        "${RC_TLSF_ROOT}"
        "${RC_TLSF_POOL_ROOT}")
    set_target_properties(rc_framework_tlsf PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)
    target_link_libraries(${PROJECT_NAME} rc_framework_tlsf)
endif()

# Only this optional backend depends on the existing board/FreeRTOS headers.
if(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS)
    add_library(rc_framework_tlsf_freertos STATIC
        "${RC_TLSF_POOL_ROOT}/tlsf_freertos_lock.cpp")
    target_link_libraries(rc_framework_tlsf_freertos PUBLIC
        rc_framework_tlsf stm32cubemx)
    set_target_properties(rc_framework_tlsf_freertos PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)
    target_link_libraries(${PROJECT_NAME} rc_framework_tlsf_freertos)
endif()
