#include "vault.h"
#include "common.h"
#include <array>
#include <filesystem>

Vault::Vault(std::string path) : m_path(std::move(path)) {
  m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
  if (!m_file.is_open()) {
    std::ofstream create(m_path, std::ios::binary);
    create.write("DULL", 4);
    create.write(reinterpret_cast<const char *>(&VERSION), sizeof(VERSION));
    create.close();
    m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
  }

  ASSERT(m_file.good());

  std::array<char, 4> header{};
  ASSERT(static_cast<bool>(m_file.read(header.data(), header.size())));
  ASSERT(std::string_view(header.data(), header.size()) == "DULL");

  std::int16_t version = 0;
  ASSERT(m_file.read(reinterpret_cast<char *>(&version), sizeof(version)));
  ASSERT(version == VERSION);
}

std::vector<FileHeader> Vault::read_file_headers() {
  std::vector<FileHeader> headers;

  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  while (true) {
    u64 name_size = 0;
    if (!m_file.read(reinterpret_cast<char *>(&name_size), sizeof(u64))) {
      break;
    }

    FileHeader header{};
    header.name.resize(name_size);
    m_file.read(header.name.data(), static_cast<i64>(name_size));

    m_file.read(reinterpret_cast<char *>(&header.size), sizeof(u64));
    m_file.seekg(static_cast<i64>(header.size), std::ios::cur);

    headers.push_back(header);
  }

  return headers;
}

std::optional<std::string> Vault::read_file(const std::string &filename) {
  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  while (true) {
    u64 name_size = 0;
    if (!m_file.read(reinterpret_cast<char *>(&name_size), sizeof(u64))) {
      break;
    }

    std::string name;
    name.resize(name_size);
    m_file.read(name.data(), static_cast<i64>(name_size));

    u64 content_size = 0;
    m_file.read(reinterpret_cast<char *>(&content_size), sizeof(u64));

    if (name == filename) {
      std::string content(content_size, '\0');
      m_file.read(content.data(), static_cast<i64>(content_size));
      return content;
    }

    m_file.seekg(static_cast<i64>(content_size), std::ios::cur);
  }

  return std::nullopt;
}

void Vault::write_file(const std::string &filename,
                       const std::string &content) {
  m_file.clear();
  m_file.seekp(0, std::ios::end);

  u64 filename_size = filename.size();
  u64 content_size = content.size();
  m_file.write(reinterpret_cast<const char *>(&filename_size), sizeof(u64));
  m_file.write(filename.data(), static_cast<i64>(filename_size));
  m_file.write(reinterpret_cast<const char *>(&content_size), sizeof(u64));
  m_file.write(content.data(), static_cast<i64>(content_size));
  m_file.flush();
}

void Vault::update_file(const std::string &filename,
                        const std::string &content) {
  m_file.clear();
  m_file.seekg(AFTER_HEADER_OFFSET, std::ios::beg);

  i64 entry_start = -1;
  u64 entry_total_size = 0;

  while (true) {
    i64 current_pos = m_file.tellg();

    u64 name_size = 0;
    if (!m_file.read(reinterpret_cast<char *>(&name_size), sizeof(u64))) {
      break;
    }

    std::string name;
    name.resize(name_size);
    m_file.read(name.data(), static_cast<i64>(name_size));

    u64 content_size = 0;
    m_file.read(reinterpret_cast<char *>(&content_size), sizeof(u64));

    if (name == filename) {
      entry_start = current_pos;
      entry_total_size = sizeof(u64) + name_size + sizeof(u64) + content_size;
      m_file.seekg(static_cast<i64>(content_size), std::ios::cur);
      break;
    }

    m_file.seekg(static_cast<i64>(content_size), std::ios::cur);
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
    m_file.write(remaining.data(), static_cast<i64>(remaining.size()));

    i64 new_size = entry_start + static_cast<i64>(remaining.size());
    m_file.flush();
    m_file.close();

    std::filesystem::resize_file(m_path, new_size);

    m_file.open(m_path, std::ios::in | std::ios::out | std::ios::binary);
    ASSERT(m_file.good());
  }

  write_file(filename, content);
}