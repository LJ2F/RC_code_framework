#include "XboxRemote.hpp"
#include "infrared_com.hpp"
#include "Hwt101.hpp"
namespace Examples {
void PollXbox(UartPort& port,XboxRemote& xbox) {
  UartPort::Packet p{};
  while(port.Read(p)) for(size_t i=0;i<p.len;++i)xbox.processByte(p.data[i],HAL_GetTick());
  const bool fresh=xbox.online(HAL_GetTick());(void)fresh; // Application chooses action.
}
void PollInfrared(UartPort& port,InfraredModule& module) {
  UartPort::Packet p{};
  while(port.Read(p)) module.UartPortRxCbHandler(p.data,p.len);
}
// Call emitData from a DIFFERENT task so ACK input keeps running.
void PollHwt101(UartPort& port,Hwt101Parser& parser) {
  UartPort::Packet p{};
  while(port.Read(p)) for(size_t i=0;i<p.len;++i) {
    if(parser.processByte(p.data[i]) && parser.lastFrameIsAngle()) {
      const float yaw=parser.yawDeg();(void)yaw;
    }
  }
}
}
