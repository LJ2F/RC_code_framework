#pragma once
#include "transfer_protocol.hpp"
#include <span>
namespace gdut {
// One communication task owns an instance. RX payload view lasts for the callback only.
template<class Algorithm> class StreamTransport {
public:
  using Manager=packet_manager<Algorithm>;
  using Packet=typename Manager::packet_t;
  using Writer=bool(*)(const uint8_t*,std::size_t,void*);
  using Handler=void(*)(uint16_t,std::span<const uint8_t>,void*);
  StreamTransport(Writer writer,void* output,Handler handler,void* input)
      : writer_(writer),output_(output),handler_(handler),input_(input) {
    manager_.set_send_function([this](const uint8_t* b,const uint8_t* e) {
      return writer_ && writer_(b,static_cast<std::size_t>(e-b),output_);
    });
    manager_.set_receive_function([this](Packet p) {
      if(handler_) handler_(p.code(),{p.body_begin(),p.body_size()},input_);
    });
  }
  StreamTransport(const StreamTransport&)=delete;
  StreamTransport& operator=(const StreamTransport&)=delete;
  Manager& manager() noexcept {return manager_;}
  bool send(uint16_t code,std::span<const uint8_t> body) {
    Packet packet(code,body.begin(),body.end(),build_packet);
    return manager_.send(packet);
  }
  void input(const uint8_t* data,std::size_t len) {
    if(data && len) manager_.receive(data,data+len);
  }
private:
  Manager manager_;
  Writer writer_;void* output_;
  Handler handler_;void* input_;
};
}
