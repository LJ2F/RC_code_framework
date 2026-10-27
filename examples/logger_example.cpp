#include "logger.hpp"
#include "memory_map.h"
namespace Examples {
static DMA_BUFFER_ATTR uint8_t logger_rx[64],logger_tx[512];
// Task only. Select this UART explicitly; serialize every writer of this port.
HAL_StatusTypeDef LogExample(UART_HandleTypeDef* handle) {
  static UartPort port(handle,logger_rx,sizeof logger_rx,logger_tx,sizeof logger_tx);
  static Logger logger(port);
  return logger.log("framework ready\r\n"); // Busy/error propagates to application.
}
}
