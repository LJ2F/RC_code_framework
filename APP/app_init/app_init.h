#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Call once after osKernelInitialize(), before osKernelStart(). */
void App_Init(void);
#ifdef __cplusplus
}
#endif
