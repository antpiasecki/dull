#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#define ASSERT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "ASSERTION FAILED at " << __FILE__ << ":" << __LINE__       \
                << "\n"                                                        \
                << #cond << std::endl;                                         \
      abort();                                                                 \
    }                                                                          \
  } while (0)

using u8 = unsigned char;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
static_assert(sizeof(float) * 8 == 32);
using f32 = float;
static_assert(sizeof(double) * 8 == 64);
using f64 = double;

inline std::string path_to_filename(const std::string &path) {
  u64 pos = path.find_last_of("/\\");
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

template <typename T> constexpr char *to_char_ptr(T *ptr) noexcept {
  return reinterpret_cast<char *>(ptr);
}

template <typename T> constexpr const char *to_char_ptr(const T *ptr) noexcept {
  return reinterpret_cast<const char *>(ptr);
}
