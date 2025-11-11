#pragma once

#include "common.h"
#include <array>
#include <botan/secmem.h>
#include <fstream>
#include <optional>

constexpr i16 VERSION = 1;
constexpr u64 AFTER_HEADER_OFFSET = 68;

// !!! REMEMBER TO UPDATE entry_total_size IN Vault::delete_file
struct FileHeader {
  std::array<u8, 24> name_nonce;
  u64 name_ciphertext_size;
  std::string name;
  std::array<u8, 24> content_nonce;
  u64 content_ciphertext_size;
};

class Vault {
public:
  explicit Vault(std::string path, const std::string &password);

  std::vector<FileHeader> read_file_headers();
  std::optional<std::string> read_file(const std::string &name);
  void create_file(const std::string &name, const std::string &content);
  void delete_file(const std::string &name);
  void update_file(const std::string &name, const std::string &content);

  const std::string &path() const { return m_path; }

private:
  std::string m_path;
  std::fstream m_file;
  Botan::secure_vector<u8> m_key;

  std::optional<FileHeader> read_file_header();
};
