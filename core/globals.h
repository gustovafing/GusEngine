#pragma once

#include <stdint.h>
#include "utils/logger.h"

extern Logger Log;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
constexpr uint64_t VARIANT_ENUM_SIZE = sizeof(short);

// Helper for enum conversions and casting

template <typename E>
constexpr int32_t enum_get_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename E>
constexpr E enum_from_underlying(int32_t v) noexcept {
    return static_cast<E>(v);
}
