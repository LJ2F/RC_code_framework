#include "UartPort.hpp"
#include "memory_map.h"
#include "cmsis_os2.h"

namespace Examples {
using BytesHandler = void(*)(const uint8_t*, size_t, void*);
static DMA_BUFFER_ATTR uint8_t rx_dma[64];
static DMA_BUFFER_ATTR uint8_t tx_dma[64];
static osSemaphoreId_t rx_ready = nullptr;
static UartPort* active_port = nullptr;

static void Notify(const uint8_t*, size_t, void*)
{
    if(rx_ready != nullptr) osSemaphoreRelease(rx_ready);
}
// Call once after osKernelInitialize() and peripheral initialization.
// The caller chooses the UART; RX must use circular DMA.
bool StartUart(UART_HandleTypeDef* handle)
{
    if(handle == nullptr || active_port != nullptr) return false;
    rx_ready = osSemaphoreNew(1U, 0U, nullptr);
    if(rx_ready == nullptr) return false;
    static UartPort port(handle, rx_dma, sizeof(rx_dma),
                         tx_dma, sizeof(tx_dma), Notify, nullptr);
    active_port = &port;
    return port.startRxDmaIdle() == HAL_OK;
}
// Call from one task. A finite wait allows protocol timeout checks elsewhere.
void PollUart(BytesHandler handler, void* user)
{
    if(active_port == nullptr || handler == nullptr) return;
    uint32_t ticks = osKernelGetTickFreq() / 100U;
    osSemaphoreAcquire(rx_ready, ticks == 0U ? 1U : ticks);
    UartPort::Packet packet{};
    while(active_port->Read(packet)) handler(packet.data, packet.len, user);
}
}
