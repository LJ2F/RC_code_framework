/**
 * @file tlsf_memory_resource.hpp
 * @author Jinfei Luo
 * @brief 
 * @version 0.1
 * @date 2026-10-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <cstddef>
#include <memory_resource>
#include "tlsf_pool.hpp"

class Tlsf_Memory_Resource final
    : public std::pmr::memory_resource {
public:
    using FailureHandler =
        void (*)(std::size_t bytes,
                 std::size_t alignment) noexcept;

    explicit Tlsf_Memory_Resource(
        Tlsf_Pool& pool,
        FailureHandler failure_handler = nullptr
    ) noexcept;

    ~Tlsf_Memory_Resource() override = default;

    Tlsf_Memory_Resource(
        const Tlsf_Memory_Resource&) = delete;

    Tlsf_Memory_Resource& operator=(
        const Tlsf_Memory_Resource&) = delete;

    Tlsf_Memory_Resource(
        Tlsf_Memory_Resource&&) = delete;

    Tlsf_Memory_Resource& operator=(
        Tlsf_Memory_Resource&&) = delete;

private:
    void* do_allocate(
        std::size_t bytes,
        std::size_t alignment
    ) override;

    void do_deallocate(
        void* pointer,
        std::size_t bytes,
        std::size_t alignment
    ) override;

    bool do_is_equal(
        const std::pmr::memory_resource& other
    ) const noexcept override;

    Tlsf_Pool& pool_;
    FailureHandler failure_handler_;
};