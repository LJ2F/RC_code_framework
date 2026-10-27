#include "check.hpp"
#include "logger.hpp"
#include "pm20s.hpp"
#include "infrared_com.hpp"
#include "XboxRemote.hpp"
#include "Hwt101.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <string>
uint32_t mock_isr=0;
static uint32_t now_tick=0,freq=100;
static HAL_StatusTypeDef pwm_status=HAL_OK, dma_status=HAL_OK,tx_status=HAL_OK;
static const uint8_t* dma_ptr=nullptr;static uint16_t dma_len=0;
static InfraredModule* infrared=nullptr;static bool fast_ack=false,recursive_send=false;
static uint8_t ack_value=0xF1;
static std::vector<uint8_t> written;
extern "C" {
uint32_t HAL_GetTick(){return now_tick;}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*,uint32_t){return pwm_status;}
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef*,const uint8_t* p,uint16_t n){dma_ptr=p;dma_len=n;return dma_status;}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*,uint8_t* p,uint16_t n,uint32_t){
 written.assign(p,p+n);
 if(recursive_send)CHECK(infrared->emitData(1,2,3)==HAL_BUSY);
 if(fast_ack){infrared->UartPortRxCbHandler(&ack_value,1);}
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
 InfraredModule ir(port);infrared=&ir;CHECK(ir.init()==HAL_OK);
 int messages=0;ir.setCallback([](const InfraredModule::Message& m,void* p){CHECK(m.address1==1 && m.address2==2 && m.data==3);++*static_cast<int*>(p);},&messages);
 uint8_t sequence[]{1,2,3,1,2,3};
 ir.UartPortRxCbHandler(sequence,1);ir.UartPortRxCbHandler(sequence+1,5);CHECK(messages==2);
 fast_ack=true;CHECK(ir.emitData(7,8,9)==HAL_OK);
 CHECK((written==std::vector<uint8_t>{0xFA,0xF1,7,8,9}));
 recursive_send=true;CHECK(ir.emitData(7,8,9)==HAL_OK);recursive_send=false;
 ack_value=0;CHECK(ir.emitData(7,8,9)==HAL_ERROR);ack_value=0xF1;
 fast_ack=false;auto before=now_tick;
 CHECK(ir.emitData(7,8,9,350)==HAL_TIMEOUT && now_tick-before==35); // 100 Hz kernel.
 now_tick=0xFFFFFFF0U;before=now_tick;
 CHECK(ir.emitData(7,8,9,350)==HAL_TIMEOUT && uint32_t(now_tick-before)==35);
 tx_status=HAL_BUSY;CHECK(ir.emitData(7,8,9)==HAL_BUSY);tx_status=HAL_OK;
 XboxRemote xbox;std::array<uint8_t,36> frame{};
 frame[0]=0xFC;frame[1]=0xFB;frame[2]=1;frame[3]=28;frame[4]=1;
 frame[20]=0x12;frame[21]=0x34;frame[34]=0xFD;frame[35]=0xFE;
 auto feed=[&](uint32_t time){int n=0;for(auto b:frame)n+=xbox.processByte(b,time);return n;};
 CHECK(feed(100)==1 && !xbox.crcVerified() && xbox.online(449) && !xbox.online(450));
 CHECK(xbox.getControllerData().btnY && xbox.getControllerData().joyLHori==0x1234);
 xbox.setChecksum([](const uint8_t* p,size_t n,void*){return n==36 && p[32]==0x42;});
 CHECK(feed(200)==0);frame[32]=0x42;CHECK(feed(200)==1 && xbox.crcVerified());
 frame[2]=2;CHECK(feed(200)==0);frame[2]=1;
 frame[3]=0;CHECK(feed(200)==0);frame[3]=28;
 xbox.processByte(0xFC,300);CHECK(feed(300)==1); // Overlapping header.
 frame[35]=0;CHECK(feed(300)==0);frame[35]=0xFE;CHECK(feed(300)==1);
 Hwt101Parser imu;std::array<uint8_t,11> angle{0x55,0x53,0,0,0,0,0,0x40,0,0,0};
 auto checksum=[&]{uint8_t s=0;for(int i=0;i<10;++i)s+=angle[i];angle[10]=s;};
 auto imufeed=[&]{int n=0;for(auto b:angle)n+=imu.processByte(b);return n;};
 checksum();CHECK(imufeed()==1 && imu.lastFrameIsAngle() && imu.yawDeg()==90 && imu.frameCount()==1);
 angle[1]=0x51;checksum();CHECK(imufeed()==1 && !imu.lastFrameIsAngle() && imu.validFrameCount()==2 && imu.frameCount()==1);
 angle[10]^=1;CHECK(imufeed()==0 && imu.validFrameCount()==2);
 angle[1]=0x53;angle[7]=0xC0;checksum();CHECK(imufeed()==1 && imu.yawDeg()==-90);
 std::puts("peripherals: passed (real UART copy path, logger, PWM, Xbox, infrared, HWT101)");
}
