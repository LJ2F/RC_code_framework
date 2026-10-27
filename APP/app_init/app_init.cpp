#include "app_init.h"
#include "main.h"
#include "bsp_dwt.h"
#include "topics.hpp"

void App_Init(void)
{
    DWT_Init(SystemCoreClock / 1000000U);
    subsQueueInit();
}
