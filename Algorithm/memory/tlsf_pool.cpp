/**
 * @file tlsf_pool.cpp
 * @author jinfui Luo
 * @brief 固定容量 TLSF 内存池实现。
 * @version 0.2
 * @date 2026-10-30
 *
 * @copyright Copyright (c) 2026
 */
#include "tlsf_pool.hpp"

#include <limits>

namespace {

bool valid_region_range(void* buffer, std::size_t bytes) noexcept {
    if (buffer == nullptr ||
        reinterpret_cast<std::uintptr_t>(buffer) % tlsf_align_size() != 0 ||
        bytes > std::numeric_limits<std::uintptr_t>::max() -
                    reinterpret_cast<std::uintptr_t>(buffer)) {
        return false;
    }
#if TLSF_MAX_POOL_SIZE > 0
    if (bytes > static_cast<std::size_t>(TLSF_MAX_POOL_SIZE)) {
        return false;
    }
#endif
    return true;
}

bool region_usable_bytes(std::size_t bytes, std::size_t& usable) noexcept {
    const std::size_t overhead = tlsf_pool_overhead();
    if (bytes < overhead) {
        return false;
    }
    const std::size_t alignment = tlsf_align_size();
    usable = ((bytes - overhead) / alignment) * alignment;
    return usable >= tlsf_block_size_min() &&
           usable < tlsf_block_size_max();
}

void collect_pool_state(
    void*, std::size_t size, int used, void* user
) noexcept {
    auto& stats = *static_cast<tlsf_pool_state*>(user);
    if (used != 0) {
        stats.used_block_bytes += size;
        ++stats.used_block;
    } else {
        stats.free_bytes += size;
        ++stats.free_block;
        if (size > stats.largest_free_block) {
            stats.largest_free_block = size;
        }
    }
}

} // 匿名命名空间结束

Tlsf_Pool::Tlsf_Pool(Tlsf_Lock* lock) noexcept
    : lock_(lock) {
}

bool Tlsf_Pool::init(void* buffer, std::size_t bytes) noexcept {
    Tlsf_Lock_Guard guard(lock_);
    const std::size_t control_bytes = tlsf_size();
    if (handle_ != nullptr ||
        !valid_region_range(buffer, bytes) ||
        bytes < control_bytes) {
        return false;
    }

    const std::size_t pool_bytes = bytes - control_bytes;
    std::size_t usable = 0;
    if (!region_usable_bytes(pool_bytes, usable)) {
        return false;
    }

    tlsf_t candidate = tlsf_create(buffer);
    if (candidate == nullptr) {
        return false;
    }

    void* pool_begin = static_cast<std::byte*>(buffer) + control_bytes;
    pool_t first_pool = tlsf_add_pool(candidate, pool_begin, pool_bytes);
    if (first_pool == nullptr) {
        tlsf_destroy(candidate);
        return false;
    }

    regions_[0] = {reinterpret_cast<std::uintptr_t>(buffer), bytes, first_pool};
    region_count_ = 1;
    usableBytes_ = usable;
    largest_region_bytes_ = usable;
    handle_ = candidate;
    return true;
}

bool Tlsf_Pool::add_region(void* buffer, std::size_t bytes) noexcept {
    Tlsf_Lock_Guard guard(lock_);
    std::size_t usable = 0;
    if (handle_ == nullptr ||
        region_count_ == max_regions ||
        !valid_region_range(buffer, bytes) ||
        !region_usable_bytes(bytes, usable) ||
        usable > std::numeric_limits<std::size_t>::max() - usableBytes_) {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(buffer);
    const auto end = begin + bytes;
    for (std::size_t i = 0; i < region_count_; ++i) {
        const auto other_end = regions_[i].begin + regions_[i].bytes;
        if (begin < other_end && regions_[i].begin < end) {
            return false;
        }
    }

    pool_t added = tlsf_add_pool(handle_, buffer, bytes);
    if (added == nullptr) {
        return false;
    }

    regions_[region_count_] = {begin, bytes, added};
    ++region_count_;
    usableBytes_ += usable;
    if (usable > largest_region_bytes_) {
        largest_region_bytes_ = usable;
    }
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
        bytes > largest_region_bytes_ ||
        alignment > largest_region_bytes_) {
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

bool Tlsf_Pool::is_initialized() const noexcept {
    Tlsf_Lock_Guard guard(lock_);
    return handle_ != nullptr;
}

Tlsf_Pool::~Tlsf_Pool() noexcept {
    // 析构必须独占进行：全部使用者须已结束，所有分配须已归还。
    if (handle_ != nullptr) {
        tlsf_destroy(handle_);
    }
}

tlsf_pool_state Tlsf_Pool::get_state() const noexcept {
    Tlsf_Lock_Guard guard(lock_);
    tlsf_pool_state stats{};
    stats.allocation_failures = allocation_failures_;
    stats.peak_used_block_bytes = peak_used_block_bytes_;
    if (handle_ == nullptr) {
        return stats;
    }

    stats.initialized = true;
    stats.capacity_bytes = usableBytes_;
    stats.region_count = region_count_;
    for (std::size_t i = 0; i < region_count_; ++i) {
        tlsf_walk_pool(regions_[i].pool, collect_pool_state, &stats);
    }
    return stats;
}

void Tlsf_Pool::record_allocation_failure() noexcept {
    if (allocation_failures_ < std::numeric_limits<std::size_t>::max()) {
        ++allocation_failures_;
    }
}