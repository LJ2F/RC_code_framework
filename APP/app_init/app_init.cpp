#include "app_init.h"
#if RC_FRAMEWORK_ENABLE_USB
#include "UsbPort.hpp"
#endif
#include "main.h"
#include "bsp_dwt.h"
#include "topics.hpp"

void App_Init(void)
{
    DWT_Init(SystemCoreClock / 1000000U);
    subsQueueInit();
#if RC_FRAMEWORK_ENABLE_USB
    if (!UsbPort::Instance().StartService()) Error_Handler();
#endif
}
