#pragma once

// Source: ROBOCON26_R2 Module/PCcom; application dispatch removed.
#include "UartPort.hpp"
#if RC_FRAMEWORK_ENABLE_USB
#include "UsbPort.hpp"
#endif
#include "transfer_protocol.hpp"
#include "verification_algorithm.hpp"
#include <cstddef>
#include <cstdint>

class PcCom {
public:
    using Packet=gdut::data_packet<gdut::crc16_algorithm>;
    using Manager=gdut::packet_manager<gdut::crc16_algorithm>;

#if RC_FRAMEWORK_ENABLE_USB
    explicit PcCom(UsbPort &usb):usb_(&usb){}
#endif
    explicit PcCom(UartPort &uart):uart_(&uart){}

    void init();
    void ProcessTx();
    void ProcessRx();

    template<typename T>
    bool send(uint16_t code,const T &msg)
    {
        const uint8_t *begin=reinterpret_cast<const uint8_t*>(&msg);
        Packet packet{
            code,
            begin,
            begin+sizeof(T),
            gdut::build_packet
        };
        if(!packet) return false;
        return manager_.send(packet);
    }

private:
    void OnPacket(Packet packet);
    Manager manager_;
    UartPort *uart_{nullptr};
#if RC_FRAMEWORK_ENABLE_USB
    UsbPort *usb_{nullptr};
#endif
};
