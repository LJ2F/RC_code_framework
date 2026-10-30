/**
 * @file tlsf_lock.hpp
 * @author Jinfei Luo
 * @brief 与操作系统无关的 TLSF 锁接口及作用域锁守卫。
 * @version 0.1
 * @date 2026-10-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

class Tlsf_Lock {
public:
    Tlsf_Lock() noexcept = default;
    virtual ~Tlsf_Lock() = default;

    Tlsf_Lock(const Tlsf_Lock&) = delete;
    Tlsf_Lock& operator=(const Tlsf_Lock&) = delete;
    Tlsf_Lock(Tlsf_Lock&&) = delete;
    Tlsf_Lock& operator=(Tlsf_Lock&&) = delete;

    virtual void lock() noexcept = 0;
    virtual void unlock() noexcept = 0;
};

class Tlsf_Lock_Guard final {
public:
    explicit Tlsf_Lock_Guard(
        Tlsf_Lock* lock = nullptr
    ) noexcept;

    ~Tlsf_Lock_Guard() noexcept;

    Tlsf_Lock_Guard(const Tlsf_Lock_Guard&) = delete;
    Tlsf_Lock_Guard& operator=(
        const Tlsf_Lock_Guard&) = delete;

    Tlsf_Lock_Guard(Tlsf_Lock_Guard&&) = delete;
    Tlsf_Lock_Guard& operator=(
        Tlsf_Lock_Guard&&) = delete;

private:
    Tlsf_Lock* lock_;
};