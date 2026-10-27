/**
 * Derived from infrared_com.hpp, zhy (Howie41), 2026-05-16.
 * 9600 baud module protocol: TX FA F1 addr1 addr2 data; RX addr1 addr2 data;
 * ACK F1. RX must be serviced by another task while emitData waits.
 */
#pragma once
#include "UartPort.hpp"
#include "cmsis_os2.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
class InfraredModule {
public:
  static constexpr uint32_t TX_MAX_TIMEOUT=1000;
  struct Message { uint8_t address1,address2,data; };
  using Callback=void(*)(const Message&,void*);
  enum class state { NOT_INITIALIZED,READY_TO_RECEIVE_DATA,AWAITING_FOR_ACK,ACK_SUCCESS,ACK_ERROR };
  explicit InfraredModule(UartPort& uart):uart_(uart){}
  HAL_StatusTypeDef init(){state_.store(state::READY_TO_RECEIVE_DATA);return HAL_OK;}
  void setCallback(Callback callback,void* user=nullptr){callback_=callback;user_=user;}
  void UartPortRxCbHandler(const uint8_t* data,size_t len) {
    if(!data || state_.load()==state::NOT_INITIALIZED) return;
    for(size_t i=0;i<len;++i) {
      auto expected=state::AWAITING_FOR_ACK;
      if(state_.compare_exchange_strong(expected,data[i]==0xF1?state::ACK_SUCCESS:state::ACK_ERROR))
        continue;
      buffer_[index_++]=data[i];
      if(index_==3) {
        const Message message{buffer_[0],buffer_[1],buffer_[2]}; index_=0;
        if(callback_) callback_(message,user_);
      }
    }
  }
  HAL_StatusTypeDef emitData(uint8_t a,uint8_t b,uint8_t data,uint32_t timeout=TX_MAX_TIMEOUT) {
    if(__get_IPSR() || osKernelGetState()!=osKernelRunning ||
       state_.load()==state::NOT_INITIALIZED) return HAL_ERROR;
    if(sending_.test_and_set()) return HAL_BUSY;
    struct Release { std::atomic_flag& f; ~Release(){f.clear();} } release{sending_};
    // Set BEFORE write: an immediate ACK can arrive while UART transmission returns.
    state_.store(state::AWAITING_FOR_ACK);
    const uint8_t frame[]{0xFA,0xF1,a,b,data};
    const uint32_t start=osKernelGetTickCount();
    const uint64_t ticks64=(uint64_t(timeout)*osKernelGetTickFreq()+999U)/1000U;
    const uint32_t ticks=static_cast<uint32_t>(ticks64>0x7FFFFFFFU?0x7FFFFFFFU:ticks64);
    const auto status=uart_.write(frame,sizeof frame,timeout);
    if(status!=HAL_OK){state_.store(state::READY_TO_RECEIVE_DATA);return status;}
    for(;;) {
      const auto value=state_.load();
      if(value==state::ACK_SUCCESS || value==state::ACK_ERROR) {
        state_.store(state::READY_TO_RECEIVE_DATA);
        return value==state::ACK_SUCCESS?HAL_OK:HAL_ERROR;
      }
      if(uint32_t(osKernelGetTickCount()-start)>=ticks) {
        state_.store(state::READY_TO_RECEIVE_DATA);return HAL_TIMEOUT;
      }
      osDelay(1);
    }
  }
private:
  UartPort& uart_;
  std::atomic<state> state_{state::NOT_INITIALIZED};
  std::atomic_flag sending_=ATOMIC_FLAG_INIT;
  uint8_t buffer_[3]{}; size_t index_{0}; // RX task only.
  Callback callback_{}; void* user_{};
};
