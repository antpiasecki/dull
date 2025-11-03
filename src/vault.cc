#include "vault.h"
#include "common.h"
#include "crypto.h"
#include <array>
#include <filesystem>

Vault::Vault(std::string path) : m_path(std::move(path)) {
  m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
  ASSERT(m_file.is_open());

  ASSERT(m_file.good());

  std::array<char, 4> header{};
  ASSERT(static_cast<bool>(m_file.read(header.data(), header.size())));
  ASSERT(std::string_view(header.data(), header.size()) == "DULL");

  i16 version = 0;
  ASSERT(m_file.read(to_char_ptr(&version), sizeof(version)));
  ASSERT(version == VERSION);
}

std::vector<FileHeader> Vault::read_file_headers() {
  std::vector<FileHeader> headers;

  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  while (true) {
    auto header = read_file_header(m_file);
    if (header) {
      headers.push_back(header.value());
      m_file.seekg(static_cast<i64>(header->size), std::ios::cur);
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
    auto header = read_file_header(m_file);
    if (!header) {
      break;
    }

    if (header->name == filename) {
      Botan::secure_vector<u8> ciphertext;
      ciphertext.resize(header->size);
      if (!m_file.read(to_char_ptr(ciphertext.data()),
                       static_cast<i64>(header->size))) {
        break;
      }

      auto plaintext = Crypto::decrypt_xchacha20_poly1305(
          ciphertext, Crypto::KEY, header->nonce);
      return std::string(to_char_ptr(plaintext.data()), plaintext.size());
    }

    m_file.seekg(static_cast<i64>(header->size), std::ios::cur);
  }

  return std::nullopt;
}

void Vault::create_file(const std::string &filename,
                        const std::string &content) {
  m_file.clear();
  m_file.seekp(0, std::ios::end);

  u64 filename_size = filename.size();

  Botan::secure_vector<u8> plaintext(content.begin(), content.end());

  auto nonce_sv = Crypto::g_rng.random_vec(24);
  std::vector<u8> nonce(nonce_sv.begin(), nonce_sv.end());

  auto ciphertext =
      Crypto::encrypt_xchacha20_poly1305(plaintext, Crypto::KEY, nonce);
  u64 ciphertext_size = ciphertext.size();

  ASSERT(m_file.write(to_char_ptr(&filename_size), sizeof(u64)));
  ASSERT(m_file.write(filename.data(), static_cast<i64>(filename_size)));
  ASSERT(m_file.write(to_char_ptr(nonce.data()), nonce.size()));
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

    auto header = read_file_header(m_file);
    if (!header) {
      break;
    }

    if (header->name == filename) {
      entry_start = current_pos;
      entry_total_size =
          sizeof(u64) + header->name.length() + 24 + sizeof(u64) + header->size;
      m_file.seekg(static_cast<i64>(header->size), std::ios::cur);
      break;
    }

    m_file.seekg(static_cast<i64>(header->size), std::ios::cur);
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

std::optional<FileHeader> Vault::read_file_header(std::fstream &file) {
  FileHeader header{};
  header.offset = file.tellg();

  u64 name_size = 0;
  if (!file.read(to_char_ptr(&name_size), sizeof(u64))) {
    return std::nullopt;
  }

  ASSERT(name_size < 10000);

  header.name.resize(name_size);
  if (!file.read(header.name.data(), static_cast<i64>(name_size))) {
    return std::nullopt;
  }

  header.nonce.resize(24);
  if (!file.read(to_char_ptr(header.nonce.data()), 24)) {
    return std::nullopt;
  }

  if (!file.read(to_char_ptr(&header.size), sizeof(u64))) {
    return std::nullopt;
  }
  return header;
}
