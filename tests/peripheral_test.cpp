#include "check.hpp"
#include "logger.hpp"
#include "pm20s.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <string>
uint32_t mock_isr=0;
static uint32_t now_tick=0,freq=100;
static HAL_StatusTypeDef pwm_status=HAL_OK, dma_status=HAL_OK,tx_status=HAL_OK;
static const uint8_t* dma_ptr=nullptr;static uint16_t dma_len=0;
static std::vector<uint8_t> written;
extern "C" {
uint32_t HAL_GetTick(){return now_tick;}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*,uint32_t){return pwm_status;}
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef*,const uint8_t* p,uint16_t n){dma_ptr=p;dma_len=n;return dma_status;}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,uint8_t* p,uint16_t n,uint32_t){
 written.assign(p,p+n);
 return tx_status;
}
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef*){return HAL_OK;}
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef*,uint8_t*,uint16_t){return HAL_OK;}
void Error_Handler(){std::abort();}
osKernelState_t osKernelGetState(){return osKernelRunning;}
uint32_t osKernelGetTickCount(){return now_tick;}
uint32_t osKernelGetTickFreq(){return freq;}
int32_t osDelay(uint32_t t){now_tick+=t;return 0;}
}
int main(){
 UART_HandleTypeDef uart{};uint8_t rx[64]{},tx[512]{},small[8]{};
 UartPort port(&uart,rx,sizeof rx,tx,sizeof tx), borrowed(&uart,rx,sizeof rx),
   undersized(&uart,rx,sizeof rx,small,sizeof small);
 CHECK(port.txCopyCapacity()==256 && borrowed.txCopyCapacity()==0 && undersized.txCopyCapacity()==4);
 Logger logger(port), unsafe(borrowed), too_small(undersized);
 CHECK(unsafe.log("x")==HAL_ERROR && too_small.log("12345")==HAL_ERROR);
 CHECK(logger.log(nullptr)==HAL_ERROR);
 std::string long_text(400,'z');
 CHECK(logger.log("%s",long_text.c_str())==HAL_OK && dma_len==255);
 CHECK(dma_ptr!=reinterpret_cast<const uint8_t*>(long_text.c_str()));
 CHECK(dma_ptr[0]=='z' && dma_ptr[254]=='z');
 CHECK(logger.log("pending")==HAL_OK);
 CHECK(logger.log("full")==HAL_BUSY);
 port.onTxCplt();port.onTxCplt();
 dma_status=HAL_BUSY;CHECK(logger.log("busy")==HAL_BUSY);dma_status=HAL_OK;
 CHECK(logger.log_priority(1,"filtered")==HAL_OK && !port.txBusy());
 logger.set_priority(1);CHECK(logger.log_priority(1,"sent")==HAL_OK);port.onTxCplt();
 TIM_TypeDef reg{};reg.ARR=19999;TIM_HandleTypeDef timer{&reg};
 PM20sServo servo(timer,TIM_CHANNEL_1,{0,180,500,2500,1000000});
 CHECK(reg.CCR[0]==0);CHECK(servo.init()==HAL_OK);
 CHECK(servo.setServoAngle(-30)==HAL_OK && reg.CCR[0]==500);
 CHECK(servo.setServoAngle(999)==HAL_OK && reg.CCR[0]==2500);
 CHECK(servo.setServoAngle(90)==HAL_OK && reg.CCR[0]==1500);
 CHECK(servo.setServoAngle(NAN)==HAL_ERROR);
 PM20sServo bad(timer,TIM_CHANNEL_1,{1,1,500,2500,1000000});CHECK(bad.init()==HAL_ERROR);
 PM20sServo wrong_channel(timer,99,{0,180,500,2500,1000000});CHECK(!wrong_channel.valid());
 reg.ARR=100;CHECK(servo.init()==HAL_ERROR);reg.ARR=19999;
 pwm_status=HAL_ERROR;CHECK(servo.init()==HAL_ERROR);
 std::puts("UART copy, logger and PWM: passed");
}
