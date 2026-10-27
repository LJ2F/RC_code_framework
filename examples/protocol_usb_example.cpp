#include "StreamTransport.hpp"
#include "UsbPort.hpp"
namespace Examples {
using UsbTransport=gdut::StreamTransport<gdut::crc16_algorithm>;
class UsbProtocol {
public:
  UsbProtocol(UsbTransport::Handler handler,void* user)
      : stream_([](const uint8_t* d,size_t n,void*) {
          return UsbPort::Instance().writeAsync(d,n)==HAL_OK;
        },nullptr,handler,user) {}
  void poll() {
    auto& port=UsbPort::Instance();
    if(!port.connected()) stream_.manager().reset_receive();
    UsbPort::Packet p{};
    while(port.Read(p)) {
      if(generation_!=p.generation) {
        stream_.manager().reset_receive();generation_=p.generation;
      }
      stream_.input(p.data,p.len);
    }
  }
  bool send(uint16_t code,std::span<const uint8_t> body){return stream_.send(code,body);}
private:
  UsbTransport stream_;
  uint32_t generation_{0};
};
}
