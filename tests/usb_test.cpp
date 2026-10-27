#include "UsbPort.hpp"
#include "UsbPortC.h"
#include "usbd_cdc_if.h"
#include "cmsis_os2.h"
#include "check.hpp"
#include <array>
#include <cstring>
uint32_t mock_isr=0;
static int critical_depth=0,submissions=0;static bool create_ok=false;
static uint8_t* held=nullptr;static uint32_t held_len=0;
static USBD_StatusTypeDef submit_result=USBD_OK;
extern "C" {
USBD_HandleTypeDef hUsbDeviceHS{};
void mock_enter(){++critical_depth;}
void mock_exit(){CHECK(critical_depth>0);--critical_depth;}
osThreadId_t osThreadNew(void(*)(void*),void*,const osThreadAttr_t* a){
 CHECK(a->stack_size==1024 && a->priority==osPriorityNormal);return create_ok?reinterpret_cast<void*>(1):nullptr;
}
osKernelState_t osKernelGetState(){return osKernelRunning;}
uint32_t osThreadFlagsSet(osThreadId_t,uint32_t){return 1;}
uint32_t osThreadFlagsWait(uint32_t,uint32_t,uint32_t){return 1;}
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef* p,uint8_t,uint8_t* d,uint32_t n){
 CHECK(critical_depth>0);++submissions;
 if(submit_result==USBD_OK){held=d;held_len=n;}
 else { // Same termination guarantee as the firmware LL adapter.
   held=nullptr;p->dev_state=USBD_STATE_DEFAULT;UsbPort_OnStateFromIsr(0,1);
 }
 return submit_result;
}
// Link-only platform stubs: abort if an unmodelled hardware/control path is used.
void* USBD_static_malloc(uint32_t){std::abort();}
void USBD_static_free(void*){std::abort();}
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef*,uint8_t,uint8_t,uint16_t){std::abort();}
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef*,uint8_t){std::abort();}
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef*,uint8_t,uint8_t*,uint32_t){std::abort();}
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef*,uint8_t){std::abort();}
USBD_StatusTypeDef USBD_CtlSendData(USBD_HandleTypeDef*,uint8_t*,uint32_t){std::abort();}
USBD_StatusTypeDef USBD_CtlPrepareRx(USBD_HandleTypeDef*,uint8_t*,uint32_t){std::abort();}
void USBD_CtlError(USBD_HandleTypeDef*,USBD_SetupReqTypedef*){std::abort();}
void* USBD_GetEpDesc(uint8_t*,uint8_t){std::abort();}
}
int main(){
 auto& port=UsbPort::Instance();USBD_CDC_HandleTypeDef cdc{};
 std::array<uint8_t,513> bytes{};bytes[0]=7;bytes[511]=9;
 CHECK(!port.StartService());create_ok=true;CHECK(port.StartService() && port.StartService());
 CHECK(!port.connected() && port.writeAsync(bytes.data(),1)==HAL_ERROR);
 CHECK(CDC_Transmit_HS(bytes.data(),1)==USBD_FAIL);
 hUsbDeviceHS.dev_state=USBD_STATE_CONFIGURED; // Class still NULL: original crash case.
 CHECK(CDC_Transmit_HS(bytes.data(),1)==USBD_FAIL && !port.connected());
 hUsbDeviceHS.classId=USBD_MAX_SUPPORTED_CLASS;
 CHECK(CDC_Transmit_HS(bytes.data(),1)==USBD_FAIL);
 hUsbDeviceHS.classId=0;hUsbDeviceHS.pClassDataCmsit[0]=&cdc;
 hUsbDeviceHS.pClassData=&cdc; // Alias deliberately irrelevant to guard.
 CHECK(USBD_Interface_fops_HS.Init()==USBD_OK);CHECK(port.connected());
 CHECK(port.writeAsync(nullptr,1)==HAL_ERROR && port.writeAsync(bytes.data(),0)==HAL_ERROR);
 CHECK(port.writeAsync(bytes.data(),513)==HAL_ERROR);
 CHECK(CDC_Transmit_HS(nullptr,1)==USBD_FAIL && CDC_Transmit_HS(bytes.data(),513)==USBD_FAIL);
 mock_isr=1;CHECK(port.writeAsync(bytes.data(),1)==HAL_ERROR);mock_isr=0;
 for(int i=0;i<8;++i)CHECK(port.writeAsync(bytes.data(),512)==HAL_OK);
 CHECK(port.writeAsync(bytes.data(),1)==HAL_BUSY && submissions==0);
 bytes[0]=99;
 port.ServiceOnce();CHECK(held && held_len==512 && held[0]==7 && held[511]==9 && port.txBusy());
 port.ServiceOnce();CHECK(submissions==1);
 auto complete=[&] {
   auto* p=held;uint32_t n=held_len;held=nullptr;cdc.TxState=0;
   mock_isr=1;USBD_Interface_fops_HS.TransmitCplt(p,&n,1);mock_isr=0;
 };
 for(int i=0;i<8;++i){complete();port.ServiceOnce();}
 CHECK(!port.txBusy() && port.getStats().tx_packets==8 && port.getStats().tx_bytes==4096);
 // Low-level BUSY retains the single complete request until retry.
 cdc.TxState=1;CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);
 port.ServiceOnce();CHECK(port.txBusy() && port.getStats().tx_busy_hits>0);
 cdc.TxState=0;port.ServiceOnce();CHECK(held_len==10);complete();port.ServiceOnce();
 // RX callback is task-owned, chunks remain transport chunks.
 int callbacks=0;port.SetRxCallback([](const uint8_t*,size_t,void* p){++*static_cast<int*>(p);},&callbacks);
 mock_isr=1;port.OnRxFromIsr(bytes.data(),513);mock_isr=0;
 CHECK(callbacks==0);UsbPort::Packet packet{};
 CHECK(port.Read(packet) && packet.len==256);
 CHECK(port.Read(packet) && packet.len==256);
 CHECK(port.Read(packet) && packet.len==1 && !port.Read(packet) && callbacks==3);
 CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);port.ServiceOnce();auto* active=held;
 CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);
 port.OnRxFromIsr(bytes.data(),10);
 hUsbDeviceHS.dev_state=USBD_STATE_SUSPENDED;UsbPort_OnStateFromIsr(0,0);
 port.ServiceOnce();CHECK(held==active && port.txBusy() && !port.connected());
 CHECK(port.writeAsync(bytes.data(),10)==HAL_ERROR && !port.Read(packet));
 hUsbDeviceHS.dev_state=USBD_STATE_CONFIGURED;UsbPort_OnStateFromIsr(1,0);
 CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);
 port.ServiceOnce();CHECK(held==active); // Resume does not recycle in-flight buffer.
 complete();port.ServiceOnce();CHECK(held_len==10);complete();port.ServiceOnce();
 // Reset/deinit terminates active data; no replay after new configuration.
 CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);port.ServiceOnce();
 CHECK(port.writeAsync(bytes.data(),10)==HAL_OK);port.OnRxFromIsr(bytes.data(),10);
 held=nullptr;cdc.TxState=0;hUsbDeviceHS.dev_state=USBD_STATE_DEFAULT;
 USBD_Interface_fops_HS.DeInit();port.ServiceOnce();
 CHECK(!port.txBusy() && !port.Read(packet));
 hUsbDeviceHS.dev_state=USBD_STATE_CONFIGURED;USBD_Interface_fops_HS.Init();
 const auto calls=submissions;port.ServiceOnce();CHECK(calls==submissions && !held);
 // Middleware's real TransmitPacket now propagates a failed LL submission.
 submit_result=USBD_FAIL;CHECK(port.writeAsync(bytes.data(),20)==HAL_OK);
 const auto errors=port.getStats().tx_errors;port.ServiceOnce();
 CHECK(port.getStats().tx_errors==errors+1 && !port.txBusy() && !held && !port.connected());
 hUsbDeviceHS.dev_state=USBD_STATE_CONFIGURED;USBD_Interface_fops_HS.Init();submit_result=USBD_OK;
 for(int i=0;i<9;++i)port.OnRxFromIsr(bytes.data(),1);
 CHECK(port.getStats().rx_drop_packets>=1 && port.getStats().tx_drop_packets>=3);
 CHECK(critical_depth==0);
 std::printf("USB: passed; static UsbPort=%zu bytes; real CDC guard and class submission tested\n",sizeof(UsbPort));
}
