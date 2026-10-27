#include "StreamTransport.hpp"
#include "UartPort.hpp"
#include "memory_map.h"
namespace Examples {
using Transport=gdut::StreamTransport<gdut::crc16_algorithm>;
// Construct in the communication task after UART init. No ISR parsing.
class UartProtocol {
public:
  UartProtocol(UartPort& port,Transport::Handler handler,void* user)
      : port_(port),stream_([](const uint8_t* d,size_t n,void* u) {
        auto& p=*static_cast<UartPort*>(u);
        return p.txCopyCapacity()>=n && p.writeDma(d,n)==HAL_OK;
      },&port,handler,user) {}
  void poll() {
    UartPort::Packet p{};
    while(port_.Read(p)) stream_.input(p.data,p.len);
  }
  bool send(uint16_t code,std::span<const uint8_t> body){return stream_.send(code,body);}
private:
  UartPort& port_;Transport stream_;
};
// A maximum 512-byte frame needs 1024 bytes of UART TX double buffering.
static DMA_BUFFER_ATTR uint8_t protocol_rx[256],protocol_tx[1024];
bool StartProtocolUart(UART_HandleTypeDef* handle) {
  static UartPort port(handle,protocol_rx,sizeof protocol_rx,protocol_tx,sizeof protocol_tx);
  return port.startRxDmaIdle()==HAL_OK; // Invoke explicitly; no default binding.
}
}
