cmake_minimum_required(VERSION 3.22)
project(RC_code_framework LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(RC_FRAMEWORK_ENABLE_USB "Enable USB CDC enumeration" ON)
option(RC_FRAMEWORK_ENABLE_MATRIX "Enable fixed-size matrices and minimal CMSIS-DSP" OFF)
option(RC_FRAMEWORK_ENABLE_TLSF "Enable the optional TLSF core and fixed-capacity C++ pool" OFF)
option(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS "Enable the optional static FreeRTOS mutex backend for TLSF" OFF)
set(RC_FRAMEWORK_TLSF_MAX_POOL_SIZE "65536" CACHE STRING "Maximum bytes per TLSF region; 0 keeps the upstream index range")
add_compile_definitions(RC_FRAMEWORK_ENABLE_MATRIX=$<BOOL:${RC_FRAMEWORK_ENABLE_MATRIX}>)
add_compile_definitions(RC_FRAMEWORK_ENABLE_USB=$<BOOL:${RC_FRAMEWORK_ENABLE_USB}>)
add_compile_definitions(RC_FRAMEWORK_ENABLE_TLSF=$<BOOL:${RC_FRAMEWORK_ENABLE_TLSF}>)
add_compile_definitions(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS=$<BOOL:${RC_FRAMEWORK_ENABLE_TLSF_FREERTOS}>)
add_executable(${PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)
include(cmake/usb.cmake)
include(cmake/matrix.cmake)
include(cmake/tlsf.cmake)
set(FRAMEWORK_INCLUDE_DIRS
    APP/app_init BSP Algorithm
    Module/controller Module/kalman Module/movebase Module/PCcom Module/imu Module/motor
    Module/motor/djiMotor Module/motor/dmMotor Module/motor/vescMotor
    Module/remote Module/topics Module/transfer_protocol
    Module/logger Module/servo Module/xbox Module/infrared_com)
target_sources(${PROJECT_NAME} PRIVATE
    APP/app_init/app_init.cpp
    BSP/bsp_dwt.c BSP/CanBus.cpp BSP/UartPort.cpp
    Module/controller/pid_controller.c
    Module/movebase/pathplanning.cpp Module/PCcom/PCcom.cpp
    Module/motor/djiMotor/djiMotor.cpp
    Module/remote/Remote_receiver.cpp Module/topics/topics.cpp
    Module/xbox/XboxRemote.cpp Module/imu/Hwt101.cpp
    Module/servo/bsp_pca9685.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${FRAMEWORK_INCLUDE_DIRS})
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
target_link_libraries(${PROJECT_NAME} stm32cubemx)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    VERBATIM)
