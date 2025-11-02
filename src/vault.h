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
  u64 offset;
};

class Vault {
public:
  explicit Vault(std::string path);

  std::vector<FileHeader> read_file_headers();
  std::optional<std::string> read_file(const std::string &name);
  void create_file(const std::string &name, const std::string &content);
  void delete_file(const std::string &name);
  void update_file(const std::string &name, const std::string &content);

  static std::optional<FileHeader> read_file_header(std::fstream &file);

private:
  std::string m_path;
  std::fstream m_file;
};
