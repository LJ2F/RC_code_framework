/**
 * @file tlsf_pool.hpp
 * @author jinfui Luo
 * @brief 借用一段或多段外部内存的固定容量 TLSF 分配器。
 * @version 0.2
 * @date 2026-10-30
 *
 * @copyright Copyright (c) 2026
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "tlsf.h"
#include "tlsf_lock.hpp"

struct tlsf_pool_state {
    bool initialized = false; // 是否已成功初始化。
    std::size_t capacity_bytes = 0; // 所有区域的初始可管理容量之和。
    std::size_t free_bytes = 0; // 当前空闲块大小之和。
    std::size_t used_block_bytes = 0; // 当前已用块内部大小之和，不等于请求字节数。
    std::size_t largest_free_block = 0; // 所有区域中最大的单个空闲块。
    std::size_t free_block = 0; // 空闲块数量。
    std::size_t used_block = 0; // 已用块数量。
    std::size_t allocation_failures = 0; // 分配失败累计次数。
    std::size_t peak_used_block_bytes = 0; // 已用块内部大小之和的历史峰值。
    std::size_t region_count = 0; // 已登记的区域数量，包含首段。
};

class Tlsf_Pool final {
public:
    /** @brief 最多登记的区域数量（含首段）；登记表不使用动态内存。 */
    static constexpr std::size_t max_regions = 8;

    /** @brief 借用可选锁，锁必须比池活得更久；nullptr 表示单任务独占使用。 */
    explicit Tlsf_Pool(Tlsf_Lock* lock = nullptr) noexcept;
    ~Tlsf_Pool() noexcept;

    Tlsf_Pool(const Tlsf_Pool&) = delete;
    Tlsf_Pool& operator=(const Tlsf_Pool&) = delete;
    Tlsf_Pool(Tlsf_Pool&&) noexcept = delete;
    Tlsf_Pool& operator=(Tlsf_Pool&&) noexcept = delete;

    /**
     * @brief 借用首段内存，并在其中建立所有区域共用的 TLSF 控制结构。
     * @return 重复初始化、地址或对齐或容量不合法、核心初始化失败时返回 false。
     * @note 所有区域必须保持可写且有效，直到池及其全部使用者结束。
     *       TLSF_MAX_POOL_SIZE 限制完整的 bytes 参数，包含控制结构和管理信息。
     */
    bool init(void* buffer, std::size_t bytes) noexcept;

    /**
     * @brief 显式加入一段不重叠的内存，复用已有控制结构。
     * @return 尚未初始化、区域不合法或重叠、登记表已满时返回 false。
     * @note 仅限任务上下文，使用同一把可选锁保护；已有分配仍然有效。
     *       不支持移除、自动扩容或指定分配区域，不保证 DMA 可访问性和缓存一致性。
     */
    bool add_region(void* buffer, std::size_t bytes) noexcept;

    /**
     * @brief 仅限任务上下文申请原始存储；请求不合法或空间不足时返回 nullptr。
     * @param alignment 非零的 2 的幂；单次分配不能跨越多个区域。
     */
    void* try_allocate(
        std::size_t bytes,
        std::size_t alignment = alignof(std::max_align_t)
    ) noexcept;

    /** @brief nullptr 无操作；其他指针必须是本池返回且尚未释放的原始地址，只能释放一次。 */
    void deallocate(void* pointer) noexcept;

    /** @brief 仅表示初始化成功，不代表仍有可分配空间。 */
    bool is_initialized() const noexcept;

    /** @brief 在共用锁内遍历所有区域和内存块，复杂度为 O(总块数 + 区域数)，应低频调用。 */
    tlsf_pool_state get_state() const noexcept;

private:
    struct Region {
        std::uintptr_t begin = 0; // 整段借入内存的起始地址，用于重叠检查。
        std::size_t bytes = 0; // 整段借入内存的长度，首段包含公共控制结构。
        pool_t pool = nullptr; // TLSF 登记的区域起点；首段已跳过控制结构。
    };

    Tlsf_Lock* lock_ = nullptr;
    tlsf_t handle_ = nullptr;
    std::size_t usableBytes_ = 0; // 所有区域的初始可管理容量总和。
    std::size_t largest_region_bytes_ = 0; // 单段初始可管理容量的最大值，并非当前空闲量。
    std::array<Region, max_regions> regions_{};
    std::size_t region_count_ = 0;

    std::size_t allocation_failures_ = 0;
    std::size_t current_used_block_bytes_ = 0;
    std::size_t peak_used_block_bytes_ = 0;

    void record_allocation_failure() noexcept;
};