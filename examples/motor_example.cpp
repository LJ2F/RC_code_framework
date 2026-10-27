#include "CanBus.hpp"
#include "djiMotor.hpp"

namespace Examples {
// Call explicitly after HAL FDCAN initialization. No default device or ID.
bool StartMotorBus(CanBus& bus, FDCAN_HandleTypeDef* handle, CanDevice& motor)
{
    if(handle == nullptr || bus.init() != CanBus::CanError::OK) return false;
    if(bus.registerDevice(&motor) != CanBus::CanError::OK) return false;
    canFilterInit(handle, FDCAN_STANDARD_ID, 0U, 0U, FDCAN_FILTER_TO_RXFIFO0, 0U);
    bspCanInit(handle);
    return true;
}
// Explicit opt-in send. This example uses direct current mode (OutputType::NONE).
// Caller supplies a configured motor, an appropriate tx group and a valid ID.
// No board, motor instance, tuned PID or repeating send task is installed.
bool SendSingleC620(CanBus& bus, C620Motor& motor, uint32_t tx_group,
                    float current_ma, bool enabled)
{
    const uint32_t id = motor.getID();
    const bool valid = (tx_group == 0x200U && id >= 0x201U && id <= 0x204U) ||
                       (tx_group == 0x1FFU && id >= 0x205U && id <= 0x208U);
    if(!valid) return false;
    motor.setMotorCmd(enabled && !motor.isOffline() ? current_ma : 0.0f);
    const int16_t command = static_cast<int16_t>(motor.cmdTrans());
    CanBus::ClassicPack pack{};
    pack.id = tx_group;
    pack.type = CanBus::Type::STANDARD;
    pack.dlc = FDCAN_DLC_BYTES_8;
    packDJIMotorCanMsg(tx_group, &id, &command, 1U, pack.data);
    return bus.addCanMsg(pack) == CanBus::CanError::OK;
}
}
