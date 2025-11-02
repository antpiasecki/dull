#pragma once

#include <cstdint>

#define ASSERT(cond)                                                           \
  if (!(cond)) {                                                               \
    std::cerr << "ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << "\n" \
              << #cond << std::endl;                                           \
    abort();                                                                   \
  }

using u8 = unsigned char;
using i8 = char;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
static_assert(sizeof(float) * 8 == 32);
using f32 = float;
static_assert(sizeof(double) * 8 == 64);
using f64 = double;