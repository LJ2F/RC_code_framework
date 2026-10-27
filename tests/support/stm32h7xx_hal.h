#pragma once
#include <stdint.h>
#include <stddef.h>
#define __IO volatile
#define __PACKED __attribute__((packed))
#define __STATIC_INLINE static inline
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {HAL_OK=0,HAL_ERROR=1,HAL_BUSY=2,HAL_TIMEOUT=3} HAL_StatusTypeDef;
typedef struct {struct {uint32_t maxpacket;} IN_ep[16];} PCD_HandleTypeDef;
typedef struct {uint32_t counter;} DMA_HandleTypeDef;
typedef struct {DMA_HandleTypeDef* hdmarx;uint32_t ErrorCode;} UART_HandleTypeDef;
typedef struct {uint32_t ARR,CCR[6];} TIM_TypeDef;
typedef struct {TIM_TypeDef* Instance;} TIM_HandleTypeDef;
enum {TIM_CHANNEL_1=0,TIM_CHANNEL_2=4,TIM_CHANNEL_3=8,TIM_CHANNEL_4=12,TIM_CHANNEL_5=16,TIM_CHANNEL_6=20};
extern uint32_t mock_isr;
uint32_t HAL_GetTick(void);
static inline uint32_t __get_IPSR(void){return mock_isr;}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef*,const uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef*,uint8_t*,uint16_t);
void HAL_Delay(uint32_t);
#define __HAL_TIM_GET_AUTORELOAD(p) ((p)->Instance->ARR)
#define __HAL_TIM_SET_COMPARE(p,c,v) ((p)->Instance->CCR[(c)/4]=(v))
#define __HAL_DMA_GET_COUNTER(p) ((p)->counter)
#define HAL_UART_ERROR_NONE 0U
#define __HAL_UART_CLEAR_PEFLAG(p) ((void)(p))
#define __HAL_UART_CLEAR_FEFLAG(p) ((void)(p))
#define __HAL_UART_CLEAR_NEFLAG(p) ((void)(p))
#define __HAL_UART_CLEAR_OREFLAG(p) ((void)(p))
#define __HAL_UART_CLEAR_IDLEFLAG(p) ((void)(p))
#define UNUSED(x) ((void)(x))
#ifdef __cplusplus
}
#endif
