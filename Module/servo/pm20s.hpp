/**
 * PM20sServo derived from pm20s.hpp, Keten (2863861004@qq.com), Copyright (c) 2026.
 * Configuration belongs to the application; construction never starts PWM.
 */
#pragma once
#include "stm32h7xx_hal.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
class PM20sServo {
public:
  struct Config { float min_angle, max_angle, min_pulse_us, max_pulse_us;
                  uint32_t counter_hz; };
  PM20sServo(TIM_HandleTypeDef& timer,uint32_t channel,Config config)
      : timer_(timer), channel_(channel), config_(config) {}
  HAL_StatusTypeDef init() {
    return valid()?HAL_TIM_PWM_Start(&timer_,channel_):HAL_ERROR;
  }
  HAL_StatusTypeDef setServoAngle(float angle) {
    if(!valid() || !std::isfinite(angle)) return HAL_ERROR;
    angle=std::clamp(angle,config_.min_angle,config_.max_angle);
    const double pulse=config_.min_pulse_us +
      (double(angle)-config_.min_angle)/(double(config_.max_angle)-config_.min_angle)*
      (double(config_.max_pulse_us)-config_.min_pulse_us);
    const auto ticks=static_cast<uint32_t>(std::round(pulse*config_.counter_hz/1000000.0));
    __HAL_TIM_SET_COMPARE(&timer_,channel_,ticks);
    return HAL_OK;
  }
  bool valid() const {
    const auto& c=config_;
    return timer_.Instance && channelValid() &&
      std::isfinite(c.min_angle) && std::isfinite(c.max_angle) &&
      std::isfinite(c.min_pulse_us) && std::isfinite(c.max_pulse_us) &&
      c.min_angle<c.max_angle && c.min_pulse_us>0 &&
      c.min_pulse_us<c.max_pulse_us && c.counter_hz>0 &&
      double(c.max_pulse_us)*c.counter_hz/1000000.0 <=
        double(__HAL_TIM_GET_AUTORELOAD(&timer_));
  }
private:
  bool channelValid() const {
    return channel_==TIM_CHANNEL_1 || channel_==TIM_CHANNEL_2 ||
      channel_==TIM_CHANNEL_3 || channel_==TIM_CHANNEL_4 ||
      channel_==TIM_CHANNEL_5 || channel_==TIM_CHANNEL_6;
  }
  TIM_HandleTypeDef& timer_;
  uint32_t channel_;
  Config config_;
};
