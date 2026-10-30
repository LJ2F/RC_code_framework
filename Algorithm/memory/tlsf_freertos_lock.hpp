/**
 * @file tlsf_freertos_lock.hpp
 * @author Jinfei Luo
 * @brief TLSF 可选的 FreeRTOS 静态互斥锁后端。
 * @version 0.1
 * @date 2026-10-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once

#if !defined(RC_FRAMEWORK_ENABLE_TLSF_FREERTOS) || !RC_FRAMEWORK_ENABLE_TLSF_FREERTOS
#error "Enable RC_FRAMEWORK_ENABLE_TLSF_FREERTOS to use this adapter."
#endif

#include "tlsf_lock.hpp"
#include "FreeRTOS.h"
#include "semphr.h"

#if configSUPPORT_STATIC_ALLOCATION != 1 || configUSE_MUTEXES != 1
#error "TLSF FreeRTOS lock requires static allocation and mutex support."
#endif
#if INCLUDE_xTaskGetSchedulerState != 1 || INCLUDE_xSemaphoreGetMutexHolder != 1 || INCLUDE_vTaskSuspend != 1
#error "TLSF FreeRTOS lock requires scheduler/holder queries and indefinite waits."
#endif

class Tlsf_FreeRTOS_Lock final : public Tlsf_Lock {
public:
    Tlsf_FreeRTOS_Lock() noexcept = default;
    ~Tlsf_FreeRTOS_Lock() override;

    Tlsf_FreeRTOS_Lock(const Tlsf_FreeRTOS_Lock&) = delete;
    Tlsf_FreeRTOS_Lock& operator=(const Tlsf_FreeRTOS_Lock&) = delete;
    Tlsf_FreeRTOS_Lock(Tlsf_FreeRTOS_Lock&&) = delete;
    Tlsf_FreeRTOS_Lock& operator=(Tlsf_FreeRTOS_Lock&&) = delete;

    /**
     * @brief 独占初始化，发布给其他任务前调用；允许在调度器启动前创建。
     * @return 重复初始化、非法上下文或创建失败时返回 false，不破坏已有句柄。
     */
    bool init() noexcept;
    /** @brief 只查询是否创建；初始化与析构不得并发进行。 */
    bool is_initialized() const noexcept;

    /**
     * @brief 仅普通运行任务；使用 portMAX_DELAY 等待，不把竞争当作内存不足。
     * @note 禁止 ISR、调度器未运行/挂起、中断屏蔽、递归获取；违约直接终止。
     */
    void lock() noexcept override;
    /** @brief 仅持有者释放；底层失败或非法上下文直接终止。 */
    void unlock() noexcept override;

private:
    StaticSemaphore_t storage_{};
    SemaphoreHandle_t handle_ = nullptr;
};
