#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void* osThreadId_t;
typedef enum {osKernelInactive=0,osKernelReady=1,osKernelRunning=2} osKernelState_t;
typedef enum {osPriorityNormal=24} osPriority_t;
typedef struct {const char* name;osPriority_t priority;uint32_t stack_size;} osThreadAttr_t;
enum {osFlagsWaitAny=0};
osThreadId_t osThreadNew(void(*)(void*),void*,const osThreadAttr_t*);
osKernelState_t osKernelGetState(void);
uint32_t osThreadFlagsSet(osThreadId_t,uint32_t);
uint32_t osThreadFlagsWait(uint32_t,uint32_t,uint32_t);
uint32_t osKernelGetTickCount(void);
uint32_t osKernelGetTickFreq(void);
int32_t osDelay(uint32_t);
#ifdef __cplusplus
}
#endif
