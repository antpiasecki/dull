#pragma once

#include "common.h"
#include <botan/secmem.h>
#include <fstream>
#include <optional>
#include <vector>

constexpr i16 VERSION = 1;
constexpr u64 AFTER_HEADER_OFFSET = 22;

struct FileHeader {
  std::string name;
  u64 name_ciphertext_size;
  u64 content_size;
  std::vector<u8> nonce;
  u64 global_offset;
};

class Vault {
public:
  explicit Vault(std::string path, const std::string &password);

  std::vector<FileHeader> read_file_headers();
  std::optional<std::string> read_file(const std::string &name);
  void create_file(const std::string &name, const std::string &content);
  void delete_file(const std::string &name);
  void update_file(const std::string &name, const std::string &content);

private:
  std::string m_path;
  std::fstream m_file;
  Botan::secure_vector<u8> m_key;

  std::optional<FileHeader> read_file_header();
};
