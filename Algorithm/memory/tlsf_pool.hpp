/**
 * @file tlsf_pool.hpp
 * @author jinfui Luo
 * @brief 固定容量、借用外部存储的 TLSF 内存池。
 * @version 0.1
 * @date 2026-10-30
 *
 * @copyright Copyright (c) 2026
 *
 */

 #pragma once

 #include<cstddef>
 #include"tlsf.h"
#include "tlsf_lock.hpp"

struct tlsf_pool_state{
    bool initialized =false; //是否初始化

    std::size_t capacity_bytes = 0; //可管理的容量上限（去除管理开销）
    std::size_t free_bytes =0; //当前的空闲容量和
    std::size_t used_block_bytes =0;//已使用的容量和
    std::size_t largest_free_block =0 ;//最大的空闲块大小

    std::size_t free_block =0 ;//空闲块数量
    std::size_t used_block =0;   //使用的块数量

    std::size_t allocation_failures = 0;//分配失败次数
    std::size_t peak_used_block_bytes =0;//峰值已使用容量和
};

class Tlsf_Pool final{
public:
    /** @brief 借用可选锁；nullptr 保留单任务模式，锁须长于池的生命周期。 */
    explicit Tlsf_Pool(Tlsf_Lock* lock = nullptr) noexcept;
    ~Tlsf_Pool() noexcept;

    Tlsf_Pool(const Tlsf_Pool&) =delete;
    Tlsf_Pool& operator=(const Tlsf_Pool&) =delete;
    Tlsf_Pool(Tlsf_Pool&&) noexcept =delete;
    Tlsf_Pool& operator=(Tlsf_Pool&&) noexcept =delete;

    /**
     * @brief 借用 buffer 指向的可写区域；区域必须覆盖池与全部分配结果的生命周期。
     * @return 初始化成功返回 true；空指针、重复初始化、对齐或容量错误返回 false。
     */
    bool init(void *buffer,std::size_t bytes)noexcept;

    
    /**
     * @brief 任务上下文调用，禁止 ISR；提供锁时统一互斥，nullptr 时仅限单任务。
     * @param alignment 非零的 2 的幂；默认满足普通基本类型对齐。
     */
    void* try_allocate(
        std::size_t bytes,
        std::size_t alignment = alignof(std::max_align_t)
    )noexcept;

    /** @brief nullptr 无操作；其他指针必须来自本池且只能释放一次，不析构对象。 */
    void deallocate(void* pointer)noexcept;

    /** @brief 仅表示初始化成功，不表示仍有可分配空间。 */
    bool is_initialized() const noexcept;

    /** @brief O(块数)状态快照；完整遍历受同一可选锁保护，应低频调用。 */
    tlsf_pool_state get_state()const noexcept;
private:
    Tlsf_Lock* lock_ = nullptr;
    tlsf_t handle_ =nullptr;
    std::size_t usableBytes_ =0;

    std::size_t allocation_failures_ =0;
    std::size_t current_used_block_bytes_ =0;
    std::size_t peak_used_block_bytes_ =0;

    void record_allocation_failure()noexcept;
};
