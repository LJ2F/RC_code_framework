/**
 * Derived from logger.hpp, zhy (Howie41), 2026-05-23.
 * Task context only. The application serializes all writers of the UART.
 */
#pragma once
#include <cstdarg>
#include <cstdio>
#include "UartPort.hpp"
class Logger {
public:
  static constexpr size_t BUFFER_LENGTH=256;
  explicit Logger(UartPort& uart):uart_(uart){}
  HAL_StatusTypeDef log(const char* format,...) {
    va_list args; va_start(args,format); auto r=output(format,args); va_end(args); return r;
  }
  HAL_StatusTypeDef log_priority(uint8_t priority,const char* format,...) {
    if(priority<priority_) return HAL_OK; // Intentionally filtered.
    va_list args; va_start(args,format); auto r=output(format,args); va_end(args); return r;
  }
  void set_priority(uint8_t priority){priority_=priority;}
private:
  HAL_StatusTypeDef output(const char* format,va_list args) {
    if(!format || __get_IPSR()) return HAL_ERROR;
    char buffer[BUFFER_LENGTH];
    const int n=std::vsnprintf(buffer,sizeof buffer,format,args);
    if(n<=0) return HAL_ERROR;
    const size_t len=n>=int(sizeof buffer)?sizeof(buffer)-1:static_cast<size_t>(n);
    // A stack buffer is safe only when the entire write is synchronously copied.
    if(uart_.txCopyCapacity()<len) return HAL_ERROR;
    return uart_.writeDma(reinterpret_cast<const uint8_t*>(buffer),len);
  }
  UartPort& uart_;
  uint8_t priority_{255};
};
