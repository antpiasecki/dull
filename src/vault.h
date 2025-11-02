#pragma once

#include "common.h"
#include <array>
#include <fstream>
#include <iostream>
#include <optional>

namespace Vault {

inline void verify_header(std::ifstream &file) {
  ASSERT(file.good());

  std::array<char, 4> header{};
  ASSERT(file.read(header.data(), header.size()));
  ASSERT(std::string_view(header.data(), header.size()) == "DULL");
}

struct FileHeader {
  std::string name;
  u64 size;
};

inline std::optional<FileHeader> read_file_header(std::ifstream &file) {
  FileHeader header{};

  u64 name_size = 0;
  if (!file.read(reinterpret_cast<char *>(&name_size), sizeof(u64))) {
    return std::nullopt;
  }

  header.name = std::string(name_size, '\0');
  file.read(header.name.data(), static_cast<i64>(name_size));

  file.read(reinterpret_cast<char *>(&header.size), sizeof(u64));

  return header;
}

inline void write_header(std::ofstream &file) { file.write("DULL", 4); }

inline void write_file(std::ofstream &file, const std::string &name,
                       const std::string &content) {
  u64 name_size = name.size();
  u64 content_size = content.size();
  file.write(reinterpret_cast<const char *>(&name_size), sizeof(u64));
  file.write(name.data(), static_cast<i64>(name_size));
  file.write(reinterpret_cast<const char *>(&content_size), sizeof(u64));
  file.write(content.data(), static_cast<i64>(content_size));
}

}; // namespace Vault