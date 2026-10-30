#include "tlsf_memory_resource.hpp"

#include <cstdlib>

Tlsf_Memory_Resource::Tlsf_Memory_Resource(
    Tlsf_Pool& pool,
    FailureHandler failure_handler
) noexcept
    : pool_(pool),
      failure_handler_(failure_handler) {
}

void* Tlsf_Memory_Resource::do_allocate(
    std::size_t bytes,
    std::size_t alignment
) {
    const std::size_t actual_bytes =
        (bytes == 0) ? 1 : bytes;

    void* pointer =
        pool_.try_allocate(actual_bytes, alignment);

    if (pointer != nullptr) {
        return pointer;
    }

    if (failure_handler_ != nullptr) {
        failure_handler_(bytes, alignment);
    }

    std::abort();
}

void Tlsf_Memory_Resource::do_deallocate(
    void* pointer,
    std::size_t,
    std::size_t
) {
    pool_.deallocate(pointer);
}

bool Tlsf_Memory_Resource::do_is_equal(
    const std::pmr::memory_resource& other
) const noexcept {
    return this == &other;
}