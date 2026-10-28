# Optional matrix backend, deliberately independent of HAL and FreeRTOS.
if(RC_FRAMEWORK_ENABLE_MATRIX)
    set(RC_MATRIX_DSP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../Middlewares/Third_Party/CMSIS-DSP)
    set(RC_MATRIX_DSP_SRC ${RC_MATRIX_DSP_ROOT}/Source/MatrixFunctions)
    add_library(rc_framework_matrix STATIC
        ${RC_MATRIX_DSP_SRC}/arm_mat_add_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_sub_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_scale_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_mult_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_trans_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_inverse_f32.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_sub_f64.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_mult_f64.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_trans_f64.c
        ${RC_MATRIX_DSP_SRC}/arm_mat_inverse_f64.c)
    target_include_directories(rc_framework_matrix PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../Algorithm
        ${RC_MATRIX_DSP_ROOT}/Include
        ${CMAKE_CURRENT_LIST_DIR}/../Drivers/CMSIS/Include)
    target_compile_definitions(rc_framework_matrix PUBLIC ARM_MATH_CM7 ARM_MATH_MATRIX_CHECK)
    target_compile_features(rc_framework_matrix PUBLIC cxx_std_23)
    # CPU/FPU/ABI flags come from the existing Cortex-M7 toolchain.
    target_link_libraries(${PROJECT_NAME} rc_framework_matrix)
endif()
