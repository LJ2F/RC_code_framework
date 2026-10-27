#pragma once
#include <cstddef>
#ifndef RC_FRAMEWORK_MAX_FRAME_SIZE
#define RC_FRAMEWORK_MAX_FRAME_SIZE 512
#endif
namespace gdut {
inline constexpr std::size_t max_frame_size = RC_FRAMEWORK_MAX_FRAME_SIZE;
inline constexpr std::size_t packet_overhead = 10;
inline constexpr std::size_t max_payload_size = max_frame_size - packet_overhead;
static_assert(max_frame_size >= packet_overhead && max_frame_size <= 65535);
}
