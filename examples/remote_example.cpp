#include "Remote_api.hpp"
#include "stm32h7xx_hal.h"

namespace Examples {
// Instance belongs to the caller's receiving task, not a fixed UART.
class RemoteInput {
public:
    void Feed(const uint8_t* bytes, size_t len)
    {
        receiver_.feed(bytes, len, HAL_GetTick());
    }
    // Call every ~10 ms even when no UART bytes arrive.
    void Poll() { receiver_.poll(HAL_GetTick()); }
    const Remote::State& State() const { return receiver_.get_state(); }
private:
    Remote::Remote_receiver receiver_{};
};
// Channel reversal, midpoint/end-point calibration and key-to-action mappings
// belong to the application. The decoder preserves raw 0..4095 values.
}
