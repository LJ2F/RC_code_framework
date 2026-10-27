/**
 * Derived from UsbPort.cpp, Keten (2863861004@qq.com), Copyright (c) 2026.
 */
#include "UsbPort.hpp"
#include "UsbPortC.h"
#include "usbd_cdc_if.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstring>
UsbPort& UsbPort::Instance() { static UsbPort port; return port; }
extern "C" uint8_t Framework_CdcReady(void);
namespace {
struct Critical { Critical(){taskENTER_CRITICAL();} ~Critical(){taskEXIT_CRITICAL();} };
void worker(void* arg) {
  auto& port=*static_cast<UsbPort*>(arg);
  for (;;) {
    port.ServiceOnce();
    osThreadFlagsWait(1U,osFlagsWaitAny,1U);
  }
}
}
bool UsbPort::StartService() {
  if(service_.load()) return true;
  osThreadAttr_t attr{}; attr.name="usb_service";
  attr.priority=osPriorityNormal; attr.stack_size=1024;
  auto id=osThreadNew(worker,this,&attr);
  service_.store(id);
  return id!=nullptr;
}
void UsbPort::wake() {
  auto id=service_.load();
  if(id && osKernelGetState()==osKernelRunning) osThreadFlagsSet(id,1U);
}
bool UsbPort::connected() const { Critical lock; return Framework_CdcReady()!=0; }
HAL_StatusTypeDef UsbPort::writeAsync(const uint8_t* data,size_t len) {
  if(__get_IPSR() || !data || !len || len>kTxPayload) return HAL_ERROR;
  uint32_t epoch;
  { Critical lock; if(!Framework_CdcReady()) return HAL_ERROR; epoch=epoch_.load(); }
  TxPacket p{}; p.len=static_cast<uint16_t>(len); p.generation=epoch;
  std::memcpy(p.data,data,len);
  if(tx_.TryPush(p)!=Algorithm::QueueError::OK) {
    ++counters_[TxDrops]; return HAL_BUSY;
  }
  wake(); return HAL_OK;
}
bool UsbPort::Read(Packet& packet) {
  while(rx_.TryPop(packet)==Algorithm::QueueError::OK) {
    if(packet.generation!=epoch_.load() || !connected()) { ++counters_[RxDrops]; continue; }
    if(callback_) callback_(packet.data,packet.len,user_);
    return true;
  }
  return false;
}
void UsbPort::SetRxCallback(RxCallback cb,void* user) { callback_=cb; user_=user; }
bool UsbPort::txBusy() const { return inflight_.load() || staging_pending_.load() || tx_.Size()!=0; }
UsbPort::Stats UsbPort::getStats() const {
  return {counters_[RxPackets].load(),counters_[RxBytes].load(),counters_[RxDrops].load(),
    counters_[TxPackets].load(),counters_[TxBytes].load(),counters_[TxDrops].load(),
    counters_[TxBusy].load(),counters_[TxErrors].load(),counters_[Changes].load()};
}
void UsbPort::ServiceOnce() {
  // USB IRQ priority 5 is masked here. No wait/delay/allocation.
  Critical lock;
  if(inflight_.load()) {
    if(active_aborted_!=aborted_.load()) {
      ++counters_[TxDrops]; inflight_.store(false); staging_valid_=false; staging_pending_.store(false);
    } else if(active_completed_!=completed_.load()) {
      ++counters_[TxPackets]; counters_[TxBytes]+=staging_.len;
      inflight_.store(false); staging_valid_=false; staging_pending_.store(false);
    } else return; // Suspend alone never releases an active buffer.
  }
  for(;;) {
    if(!staging_valid_) {
      if(tx_.TryPop(staging_)!=Algorithm::QueueError::OK) return;
      staging_valid_=true; staging_pending_.store(true);
    }
    if(staging_.generation!=epoch_.load() || !Framework_CdcReady()) {
      ++counters_[TxDrops]; staging_valid_=false; staging_pending_.store(false); continue;
    }
    active_completed_=completed_.load(); active_aborted_=aborted_.load();
    const auto status=CDC_Transmit_HS(staging_.data,staging_.len);
    if(status==USBD_OK) { inflight_.store(true); return; }
    if(status==USBD_BUSY) { ++counters_[TxBusy]; return; }
    ++counters_[TxErrors]; ++counters_[TxDrops]; staging_valid_=false; staging_pending_.store(false);
  }
}
void UsbPort::OnRxFromIsr(const uint8_t* data,size_t len) {
  if(!data) return;
  while(len) {
    Packet p{}; const size_t n=len>kRxPayload?kRxPayload:len;
    p.len=static_cast<uint16_t>(n); p.generation=epoch_.load();
    std::memcpy(p.data,data,n);
    if(rx_.TryPush(p)!=Algorithm::QueueError::OK) { ++counters_[RxDrops]; break; }
    ++counters_[RxPackets]; counters_[RxBytes]+=n;
    data+=n; len-=n;
  }
  wake();
}
void UsbPort::OnTxCpltFromIsr() { ++completed_; wake(); }
void UsbPort::OnStateFromIsr(bool ready,bool aborted) {
  (void)ready; // Actual readiness comes from the CDC backend under IRQ exclusion.
  ++epoch_; ++counters_[Changes];
  if(aborted) ++aborted_;
  wake();
}
extern "C" void UsbPort_Prepare(){ (void)UsbPort::Instance(); }
extern "C" void UsbPort_OnRxFromIsr(const uint8_t* d,uint32_t n){ UsbPort::Instance().OnRxFromIsr(d,n); }
extern "C" void UsbPort_OnTxCpltFromIsr(){ UsbPort::Instance().OnTxCpltFromIsr(); }
extern "C" void UsbPort_OnStateFromIsr(uint8_t ready,uint8_t aborted){ UsbPort::Instance().OnStateFromIsr(ready,aborted); }
