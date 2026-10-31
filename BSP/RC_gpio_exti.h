// 来源：RC10_LIB-FrameWork，提交 07e87639a9a8e66187d8e69bf8b1fe0f2ff5553b，RC10_LIB/Module。
// 原接口约定：pin 必须是非零的单个 GPIO_PIN_x 位掩码；每条 EXTI 线一个对象。
// 在中断启用前构造，注册后对象须一直存活；原实现没有注销或并发注册接口。
// EXTI_Prosess 在中断上下文执行，只做短操作或 ISR 安全通知。
#pragma once
#include "gpio.h"

#define MAX_GPIO_EXTI_NUM 16

#ifdef __cplusplus
namespace gpio
{
	class GpioExti
    {
    public:
		GpioExti(uint16_t gpio_pin_);
		~GpioExti() = default;
		
		static void All_EXTI_Prosess(uint16_t gpio_pin_);
		
    protected:
		virtual void EXTI_Prosess() = 0;
		
    private:
		uint16_t gpio_pin;
	
		static GpioExti* gpio_exti_list[MAX_GPIO_EXTI_NUM];
    };
}
#endif

