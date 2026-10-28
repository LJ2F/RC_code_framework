#include "PCcom.hpp"

void PcCom::init()
{
    manager_.set_send_function([this](const uint8_t *begin,
                                     const uint8_t *end)noexcept -> bool {
        const size_t len=static_cast<size_t>(end-begin);
        if(uart_!=nullptr){
            return uart_->write(begin, len)==HAL_OK;
        }
#if RC_FRAMEWORK_ENABLE_USB
        else if(usb_!=nullptr){
            return usb_->writeAsync(begin, len)==HAL_OK;
        }
#endif
        return false;
    });

    manager_.set_receive_function([this](Packet packet)noexcept{
        OnPacket(std::move(packet));
    });
}

void PcCom::ProcessRx()
{
#if RC_FRAMEWORK_ENABLE_USB
    if (usb_ != nullptr) {
        UsbPort::Packet rx{};
        while (usb_->Read(rx)) {
            manager_.receive(rx.data, rx.data + rx.len);
        }
    }
#endif
    if (uart_ != nullptr) {
        UartPort::Packet rx{};
        while (uart_->Read(rx)) {
            manager_.receive(rx.data, rx.data + rx.len);
        }
    }
}

void PcCom::OnPacket(Packet packet)
{
    // Application-specific command dispatch belongs here.
    (void)packet;
}

void PcCom::ProcessTx()
{
    // Application-specific outgoing messages belong here.
}
