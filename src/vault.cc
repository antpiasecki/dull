#include "vault.h"
#include "common.h"
#include "crypto.h"
#include <array>
#include <botan/auto_rng.h>
#include <filesystem>

Vault::Vault(std::string path, const std::string &password)
    : m_path(std::move(path)) {
  m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
  ASSERT(m_file.good());

  std::array<char, 4> magic{};
  ASSERT(m_file.read(magic.data(), 4));
  ASSERT(std::string_view(magic.data(), magic.size()) == "DULL");

  i16 version = 0;
  ASSERT(m_file.read(to_char_ptr(&version), sizeof(version)));
  ASSERT(version == VERSION);

  std::vector<u8> salt{};
  salt.resize(16);
  ASSERT(m_file.read(to_char_ptr(salt.data()), 16));

  m_key = Crypto::derive_key_argon2id(password, salt);
}

std::vector<FileHeader> Vault::read_file_headers() {
  std::vector<FileHeader> headers;

  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  while (true) {
    auto header = read_file_header();
    if (header) {
      headers.push_back(header.value());
      m_file.seekg(static_cast<i64>(header->content_size), std::ios::cur);
    } else {
      break;
    }
  }

  return headers;
}

std::optional<std::string> Vault::read_file(const std::string &filename) {
  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  while (true) {
    auto header = read_file_header();
    if (!header) {
      break;
    }

    if (header->name == filename) {
      Botan::secure_vector<u8> ciphertext;
      ciphertext.resize(header->content_size);
      if (!m_file.read(to_char_ptr(ciphertext.data()),
                       static_cast<i64>(header->content_size))) {
        break;
      }

      auto plaintext = Crypto::decrypt_xchacha20_poly1305(
          ciphertext, m_key, header->content_nonce);
      return std::string(to_char_ptr(plaintext.data()), plaintext.size());
    }

    m_file.seekg(static_cast<i64>(header->content_size), std::ios::cur);
  }

  return std::nullopt;
}

void Vault::create_file(const std::string &filename,
                        const std::string &content) {
  m_file.clear();
  m_file.seekp(0, std::ios::end);

  static Botan::AutoSeeded_RNG rng;
  auto name_nonce_sv = rng.random_vec(24);
  std::vector<u8> name_nonce(name_nonce_sv.begin(), name_nonce_sv.end());

  auto content_nonce_sv = rng.random_vec(24);
  std::vector<u8> content_nonce(content_nonce_sv.begin(),
                                content_nonce_sv.end());

  Botan::secure_vector<u8> filename_sv(filename.begin(), filename.end());
  auto filename_ciphertext =
      Crypto::encrypt_xchacha20_poly1305(filename_sv, m_key, name_nonce);
  u64 filename_ciphertext_size = filename_ciphertext.size();

  Botan::secure_vector<u8> content_sv(content.begin(), content.end());
  auto ciphertext =
      Crypto::encrypt_xchacha20_poly1305(content_sv, m_key, content_nonce);
  u64 ciphertext_size = ciphertext.size();

  ASSERT(m_file.write(to_char_ptr(name_nonce.data()), name_nonce.size()));
  ASSERT(m_file.write(to_char_ptr(&filename_ciphertext_size), sizeof(u64)));
  ASSERT(m_file.write(to_char_ptr(filename_ciphertext.data()),
                      static_cast<i64>(filename_ciphertext_size)));
  ASSERT(m_file.write(to_char_ptr(content_nonce.data()), content_nonce.size()));
  ASSERT(m_file.write(to_char_ptr(&ciphertext_size), sizeof(u64)));
  ASSERT(m_file.write(to_char_ptr(ciphertext.data()),
                      static_cast<i64>(ciphertext_size)));
  m_file.flush();
}

void Vault::delete_file(const std::string &filename) {
  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  i64 entry_start = -1;
  u64 entry_total_size = 0;

  while (true) {
    i64 current_pos = m_file.tellg();

    auto header = read_file_header();
    if (!header) {
      break;
    }

    if (header->name == filename) {
      entry_start = current_pos;
      entry_total_size = 24 + sizeof(u64) + header->name_ciphertext_size +
                         sizeof(u64) + header->content_size;
      m_file.seekg(static_cast<i64>(header->content_size), std::ios::cur);
      break;
    }

    m_file.seekg(static_cast<i64>(header->content_size), std::ios::cur);
  }

  if (entry_start != -1) {
    i64 entry_end = entry_start + static_cast<i64>(entry_total_size);

    m_file.clear();
    m_file.seekg(entry_end, std::ios::beg);
    std::string remaining;
    remaining.assign(std::istreambuf_iterator<char>(m_file),
                     std::istreambuf_iterator<char>());

    m_file.clear();
    m_file.seekp(entry_start, std::ios::beg);
    ASSERT(m_file.write(remaining.data(), static_cast<i64>(remaining.size())));

    i64 new_size = entry_start + static_cast<i64>(remaining.size());
    m_file.flush();
    m_file.close();

    std::filesystem::resize_file(m_path, new_size);

    m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
    ASSERT(m_file.good());
  }
}

void Vault::update_file(const std::string &filename,
                        const std::string &content) {
  delete_file(filename);
  create_file(filename, content);
}

std::optional<FileHeader> Vault::read_file_header() {
  FileHeader header{};

  std::vector<u8> name_nonce;
  name_nonce.resize(24);
  if (!m_file.read(to_char_ptr(name_nonce.data()), 24)) {
    return std::nullopt;
  }

  if (!m_file.read(to_char_ptr(&header.name_ciphertext_size), sizeof(u64))) {
    return std::nullopt;
  }

  ASSERT(header.name_ciphertext_size < 10000);

  Botan::secure_vector<u8> name_ciphertext;
  name_ciphertext.resize(header.name_ciphertext_size);
  if (!m_file.read(to_char_ptr(name_ciphertext.data()),
                   static_cast<i64>(header.name_ciphertext_size))) {
    return std::nullopt;
  }

  auto name =
      Crypto::decrypt_xchacha20_poly1305(name_ciphertext, m_key, name_nonce);
  header.name = std::string(name.begin(), name.end());

  header.content_nonce.resize(24);
  if (!m_file.read(to_char_ptr(header.content_nonce.data()), 24)) {
    return std::nullopt;
  }

  if (!m_file.read(to_char_ptr(&header.content_size), sizeof(u64))) {
    return std::nullopt;
  }
  return header;
}