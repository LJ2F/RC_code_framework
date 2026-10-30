/**
 * @file tlsf_lock.cpp
 * @author Jinfei Luo
 * @brief 
 * @version 0.1
 * @date 2026-10-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "tlsf_lock.hpp"

Tlsf_Lock_Guard::Tlsf_Lock_Guard(
    Tlsf_Lock* lock
) noexcept
    : lock_(lock) {
    if (lock_ != nullptr) {
        lock_->lock();
    }
}

Tlsf_Lock_Guard::~Tlsf_Lock_Guard() noexcept {
    if (lock_ != nullptr) {
        lock_->unlock();
    }
}