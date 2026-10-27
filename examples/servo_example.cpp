#include "pm20s.hpp"
namespace Examples {
// Timer clock/prescaler/ARR and GPIO alternate function are application-owned.
// Nothing here starts PWM unless the application explicitly calls init().
PM20sServo ConfigureServo(TIM_HandleTypeDef& timer,uint32_t channel,
                         PM20sServo::Config config) {
  return PM20sServo(timer,channel,config);
}
}
