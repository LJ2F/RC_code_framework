if(RC_FRAMEWORK_ENABLE_USB)
  set(USB_ROOT ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library)
  target_include_directories(stm32cubemx INTERFACE
    ${CMAKE_SOURCE_DIR}/BSP ${CMAKE_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_SOURCE_DIR}/USB_DEVICE/Target ${USB_ROOT}/Core/Inc ${USB_ROOT}/Class/CDC/Inc)
  target_sources(${PROJECT_NAME} PRIVATE
    BSP/UsbPort.cpp USB_DEVICE/App/usb_device.c USB_DEVICE/App/usbd_desc.c
    USB_DEVICE/App/usbd_cdc_if.c USB_DEVICE/Target/usbd_conf.c
    ${USB_ROOT}/Core/Src/usbd_core.c ${USB_ROOT}/Core/Src/usbd_ctlreq.c
    ${USB_ROOT}/Core/Src/usbd_ioreq.c ${USB_ROOT}/Class/CDC/Src/usbd_cdc.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c)
endif()
