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
add_compile_definitions(RC_FRAMEWORK_ENABLE_USB=$<BOOL:${RC_FRAMEWORK_ENABLE_USB}>)
add_executable(${PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)
include(cmake/usb.cmake)
set(FRAMEWORK_INCLUDE_DIRS
    APP/app_init BSP Algorithm
    Module/controller Module/imu Module/motor
    Module/motor/djiMotor Module/motor/dmMotor Module/motor/vescMotor
    Module/remote Module/topics Module/transfer_protocol
    Module/logger Module/servo Module/xbox Module/infrared_com)
target_sources(${PROJECT_NAME} PRIVATE
    APP/app_init/app_init.cpp
    BSP/bsp_dwt.c BSP/CanBus.cpp BSP/UartPort.cpp
    Module/controller/pid_controller.c
    Module/motor/djiMotor/djiMotor.cpp
    Module/remote/Remote_receiver.cpp Module/topics/topics.cpp
    Module/xbox/XboxRemote.cpp Module/imu/Hwt101.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${FRAMEWORK_INCLUDE_DIRS})
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
target_link_libraries(${PROJECT_NAME} stm32cubemx)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    VERBATIM)

# Compile-only checks: these objects are never linked into the default firmware.
file(GLOB HEADER_CHECK_SOURCES CONFIGURE_DEPENDS checks/*.cpp checks/*.c)
add_library(framework_header_check OBJECT EXCLUDE_FROM_ALL ${HEADER_CHECK_SOURCES})
target_include_directories(framework_header_check PRIVATE ${FRAMEWORK_INCLUDE_DIRS})
target_link_libraries(framework_header_check PRIVATE stm32cubemx)
add_library(framework_example_check OBJECT EXCLUDE_FROM_ALL
    examples/topics_example.cpp examples/uart_example.cpp
    examples/remote_example.cpp examples/motor_example.cpp)
target_include_directories(framework_example_check PRIVATE ${FRAMEWORK_INCLUDE_DIRS})
target_link_libraries(framework_example_check PRIVATE stm32cubemx)

target_sources(framework_example_check PRIVATE
  examples/protocol_uart_example.cpp examples/logger_example.cpp
  examples/servo_example.cpp examples/device_protocol_example.cpp)
if(RC_FRAMEWORK_ENABLE_USB)
  target_sources(framework_example_check PRIVATE examples/protocol_usb_example.cpp)
endif()
