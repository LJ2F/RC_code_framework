/**
 * @file tlsf_pool.cpp
 * @author jinfui Luo
 * @brief 固定容量 TLSF 内存池实现。
 * @version 0.1
 * @date 2026-10-30
 *
 * @copyright Copyright (c) 2026
 *
 */

 #include"tlsf_pool.hpp"
#include "tlsf.h"
#include <cstddef>
 #include<cstdint>
#include<limits>

Tlsf_Pool::Tlsf_Pool(Tlsf_Lock* lock) noexcept
    : lock_(lock) {
}

 namespace{
    /**
     * @brief 
     * @param void* 当前的内存块地址
     * @param size 当前的块内部大小
     * @param used 非零为已使用
     * @param user 附加指针
     */
    void collect_pool_state(void*,std::size_t size,int used, void* user)noexcept
    {
        auto& stats = *static_cast<tlsf_pool_state*>(user);
        if(used!=0)
        {
            stats.used_block_bytes+=size;
            stats.used_block++;
        }else{
            stats.free_bytes +=size;
            stats.free_block++;
            if(size>stats.largest_free_block)
            {
                stats.largest_free_block =size;
            }
        }
    }
    
 }
 bool Tlsf_Pool::init(void* buffer,std::size_t bytes)noexcept{
    Tlsf_Lock_Guard guard(lock_);
    if(handle_ != nullptr||buffer==nullptr)
    {
        return false;
    }

    const std::size_t alignment = tlsf_align_size();
    const std::size_t control_bytes= tlsf_size();
    const std::size_t overhead = tlsf_pool_overhead();

    if(reinterpret_cast<std::uintptr_t>(buffer)%alignment!=0||bytes<control_bytes)
    {
        return false;
    }

    const std::size_t pool_bytes = bytes -control_bytes;
    if(pool_bytes<overhead)
    {
        return false;
    }

    const std::size_t usable = ((pool_bytes-overhead)/alignment)*alignment;
    if(usable<tlsf_block_size_min()||
        usable>=tlsf_block_size_max())
    {
        return false;
    }

    tlsf_t candidate = tlsf_create(buffer);
    if(candidate==nullptr)
    {
        return false;
    }

    void* pool_begin =static_cast<std::byte*>(buffer)+control_bytes;
    if(tlsf_add_pool(candidate, pool_begin, pool_bytes)==nullptr)
    {
        tlsf_destroy(candidate);
        return false;
    }

    handle_=candidate;
    usableBytes_=usable;
    return true;
 }

void* Tlsf_Pool::try_allocate(
    std::size_t bytes,
    std::size_t alignment
) noexcept {
    Tlsf_Lock_Guard guard(lock_);
    if (handle_ == nullptr ||
        bytes == 0 ||
        alignment == 0 ||
        (alignment & (alignment - 1)) != 0 ||
        bytes > usableBytes_ ||
        alignment > usableBytes_) {
        record_allocation_failure();
        return nullptr;
    }

    void* pointer = tlsf_memalign(handle_, alignment, bytes);

    if (pointer == nullptr) {
        record_allocation_failure();
        return nullptr;
    }

    current_used_block_bytes_ += tlsf_block_size(pointer);

    if (current_used_block_bytes_ > peak_used_block_bytes_) {
        peak_used_block_bytes_ = current_used_block_bytes_;
    }

    return pointer;
}

void Tlsf_Pool::deallocate(void* pointer) noexcept {
    Tlsf_Lock_Guard guard(lock_);
    if (handle_ != nullptr && pointer != nullptr) {
        current_used_block_bytes_ -= tlsf_block_size(pointer);
        tlsf_free(handle_, pointer);
    }
}
 bool Tlsf_Pool::is_initialized()const noexcept{
    Tlsf_Lock_Guard guard(lock_);
    return handle_!=nullptr;
 }

 Tlsf_Pool::~Tlsf_Pool() noexcept{
    if(handle_ != nullptr)
    {
        tlsf_destroy(handle_);
    }
 }


tlsf_pool_state Tlsf_Pool::get_state()const noexcept{
    Tlsf_Lock_Guard guard(lock_);
    tlsf_pool_state stats{};

    stats.allocation_failures = allocation_failures_;
    stats.peak_used_block_bytes = peak_used_block_bytes_;
    
    if(handle_ == nullptr)
    {
        return stats;
    }

    stats.initialized=true;
    stats.capacity_bytes =usableBytes_;
    tlsf_walk_pool(tlsf_get_pool(handle_), collect_pool_state, &stats);
    
    return stats;
}

void Tlsf_Pool::record_allocation_failure() noexcept {
    if (allocation_failures_ <
        std::numeric_limits<std::size_t>::max()) {
        ++allocation_failures_;
    }
}