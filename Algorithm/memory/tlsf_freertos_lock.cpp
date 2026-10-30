#include "tlsf_freertos_lock.hpp"

#include "task.h"
#include "stm32h7xx.h"
#include <cstdlib>

namespace {

bool interrupts_unmasked() noexcept {
    return __get_PRIMASK() == 0U &&
           __get_BASEPRI() == 0U &&
           __get_FAULTMASK() == 0U;
}

bool lifecycle_context() noexcept {
    if (xPortIsInsideInterrupt() != pdFALSE) {
        return false;
    }
    const BaseType_t state = xTaskGetSchedulerState();
    return state == taskSCHEDULER_NOT_STARTED ||
           (state == taskSCHEDULER_RUNNING && interrupts_unmasked());
}

bool running_task_context() noexcept {
    return xPortIsInsideInterrupt() == pdFALSE &&
           xTaskGetSchedulerState() == taskSCHEDULER_RUNNING &&
           interrupts_unmasked();
}

} // namespace

bool Tlsf_FreeRTOS_Lock::init() noexcept {
    if (handle_ != nullptr || !lifecycle_context()) {
        return false;
    }
    handle_ = xSemaphoreCreateMutexStatic(&storage_);
    return handle_ != nullptr;
}

bool Tlsf_FreeRTOS_Lock::is_initialized() const noexcept {
    return handle_ != nullptr;
}

void Tlsf_FreeRTOS_Lock::lock() noexcept {
    if (handle_ == nullptr || !running_task_context()) {
        std::abort();
    }
    const TaskHandle_t current = xTaskGetCurrentTaskHandle();
    if (current == nullptr || xSemaphoreGetMutexHolder(handle_) == current) {
        std::abort();
    }
    if (xSemaphoreTake(handle_, portMAX_DELAY) != pdTRUE) {
        std::abort();
    }
}

void Tlsf_FreeRTOS_Lock::unlock() noexcept {
    if (handle_ == nullptr || !running_task_context() ||
        xSemaphoreGetMutexHolder(handle_) != xTaskGetCurrentTaskHandle()) {
        std::abort();
    }
    if (xSemaphoreGive(handle_) != pdTRUE) {
        std::abort();
    }
}

Tlsf_FreeRTOS_Lock::~Tlsf_FreeRTOS_Lock() {
    if (handle_ == nullptr) {
        return;
    }
    // 析构由调用者保证独占；不允许其他任务持有或等待此互斥锁。
    if (!lifecycle_context() ||
        xSemaphoreGetMutexHolder(handle_) != nullptr ||
        uxSemaphoreGetCount(handle_) != 1U) {
        std::abort();
    }
    vSemaphoreDelete(handle_);
}
