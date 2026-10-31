// 来源：gdut-robocon11-second-group/robocon-library，提交 d21b3ccf822901dd22ba1e319298a8bdae3eaff7。
// 按用户确认提取 PCA9685 所需的轻量 H7 适配；保留原阻塞接口和超时转换。
// 只借用外部配置好的 HAL 句柄，不初始化外设或接管引脚；句柄须长于使用者。
// 在任务中调用；同一总线的多个使用者由应用统一串行化。
#ifndef BSP_IIC_HPP
#define BSP_IIC_HPP

#include "stm32h7xx_hal.h"
#include "uncopyable.hpp"
#include <chrono>
#include <cstdint>
#include <limits>

namespace gdut {
class i2c : private uncopyable {
public:
  explicit i2c(I2C_HandleTypeDef *hi2c) : m_hi2c(hi2c) {}
  ~i2c() noexcept = default;
  // 禁止拷贝和移动
  i2c(const i2c &) = delete;
  i2c &operator=(const i2c &) = delete;
  i2c(i2c &&) = delete;
  i2c &operator=(i2c &&) = delete;

  // 存储器写（阻塞）
  HAL_StatusTypeDef mem_write(
      uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size,
      const uint8_t *data, uint16_t size,
      std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
    return m_hi2c ? HAL_I2C_Mem_Write(m_hi2c, dev_addr, mem_addr, mem_addr_size,
                                      const_cast<uint8_t *>(data), size,
                                      timeout_cast(timeout))
                  : HAL_ERROR;
  }

  // 存储器读（阻塞）
  HAL_StatusTypeDef mem_read(
      uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size,
      uint8_t *data, uint16_t size,
      std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
    return m_hi2c ? HAL_I2C_Mem_Read(m_hi2c, dev_addr, mem_addr, mem_addr_size,
                                     data, size, timeout_cast(timeout))
                  : HAL_ERROR;
  }

  // 检查设备是否就绪
  HAL_StatusTypeDef is_device_ready(
      uint16_t dev_addr, uint32_t trials = 3,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(10)) {
    return m_hi2c ? HAL_I2C_IsDeviceReady(m_hi2c, dev_addr, trials,
                                          timeout_cast(timeout))
                  : HAL_ERROR;
  }

private:
  I2C_HandleTypeDef *m_hi2c{nullptr};

  // 超时转换
  static uint32_t timeout_cast(std::chrono::milliseconds timeout) {
    if (timeout.count() > std::numeric_limits<uint32_t>::max()) {
      return HAL_MAX_DELAY;
    }
    return static_cast<uint32_t>(timeout.count());
  }
};
} // namespace gdut

#endif // BSP_IIC_HPP
