#pragma once

#include "common.h"
#include <fstream>
#include <optional>
#include <vector>

constexpr i16 VERSION = 1;
constexpr u64 AFTER_HEADER_OFFSET = 6;

struct FileHeader {
  std::string name;
  u64 size;
};

class Vault {
public:
  explicit Vault(std::string path);

  std::vector<FileHeader> read_file_headers();
  std::optional<std::string> read_file(const std::string &name);
  void write_file(const std::string &name, const std::string &content);
  void update_file(const std::string &name, const std::string &content);

private:
  std::string m_path;
  std::fstream m_file;
};
