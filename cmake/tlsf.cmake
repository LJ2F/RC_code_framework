# 核心同步接口保持独立，不依赖 HAL 或 FreeRTOS。
if(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS AND NOT RC_FRAMEWORK_ENABLE_TLSF)
    message(FATAL_ERROR "RC_FRAMEWORK_ENABLE_TLSF_FREERTOS requires RC_FRAMEWORK_ENABLE_TLSF=ON")
endif()

if(NOT RC_FRAMEWORK_TLSF_MAX_POOL_SIZE MATCHES "^(0|[1-9][0-9]*)$")
    message(FATAL_ERROR "RC_FRAMEWORK_TLSF_MAX_POOL_SIZE must be a decimal byte count")
endif()
string(LENGTH "${RC_FRAMEWORK_TLSF_MAX_POOL_SIZE}" RC_TLSF_SIZE_DIGITS)
if(RC_TLSF_SIZE_DIGITS GREATER 10)
    message(FATAL_ERROR "RC_FRAMEWORK_TLSF_MAX_POOL_SIZE exceeds the supported range")
endif()
if(NOT RC_FRAMEWORK_TLSF_MAX_POOL_SIZE STREQUAL "0" AND
   (RC_FRAMEWORK_TLSF_MAX_POOL_SIZE LESS 1024 OR
    RC_FRAMEWORK_TLSF_MAX_POOL_SIZE GREATER 1073741824))
    message(FATAL_ERROR "RC_FRAMEWORK_TLSF_MAX_POOL_SIZE must be 0 or 1024..1073741824")
endif()

# 不自动创建内存区域、任务或默认分配器。
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
    target_compile_definitions(rc_framework_tlsf PUBLIC
        TLSF_MAX_POOL_SIZE=${RC_FRAMEWORK_TLSF_MAX_POOL_SIZE})
    set_target_properties(rc_framework_tlsf PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS NO
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)
    target_link_libraries(${PROJECT_NAME} rc_framework_tlsf)
endif()

# 仅此可选后端依赖现有板级与 FreeRTOS 头文件。
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