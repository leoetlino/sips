// Copyright 2019 leoetlino <leo@leolam.fr>
// Licensed under GPLv2+

#pragma once

#include <cstdio>
#include <utility>

#ifdef _WIN32
unsigned int htonl(unsigned int);
#else
#include <arpa/inet.h>
#endif

#include "common_types.h"
#include "swap.h"

namespace util {
namespace detail {
inline bool isBigEndianPlatform() {
  return htonl(0x12345678) == 0x12345678;
}

template <typename T>
T swapIfNeeded(T value, bool bigEndian) {
  if (isBigEndianPlatform() != bigEndian)
    value = swapValue(value);
  return value;
}
}  // namespace detail

enum class Endianness {
  Little,
  Big,
};

class File {
public:
  File(const char* path, const char* modes) {
    m_file = std::fopen(path, modes);
  }

  ~File() {
    if (m_file)
      std::fclose(m_file);
  }

  File(const File&) = delete;
  File& operator=(const File&) = delete;
  File(File&& other) {
    m_file = other.m_file;
    other.m_file = nullptr;
  }
  File& operator=(File&& other) {
    *this = File{std::move(other)};
    return *this;
  }

  explicit operator bool() const { return m_file != nullptr; }

  bool read(u8* ptr, size_t count) const {
    return std::fread(ptr, 1, count, m_file) == count;
  }

  bool write(const u8* ptr, size_t count) const {
    return std::fwrite(ptr, 1, count, m_file) == count;
  }

  template <typename T>
  bool writeArray(const T* ptr, size_t count) const {
    return write(reinterpret_cast<const u8*>(ptr), count * sizeof(T));
  }

  template <typename T>
  bool writeValue(T value, Endianness endianness = Endianness::Big) const {
    value = detail::swapIfNeeded(value, endianness == Endianness::Big);
    return writeArray(&value, 1);
  }

  size_t size() const {
    const u64 pos = std::ftell(m_file);
    if (std::fseek(m_file, 0, SEEK_END) != 0)
      return 0;

    const u64 size = std::ftell(m_file);
    if (std::fseek(m_file, pos, SEEK_SET) != 0)
      return 0;

    return size;
  }

private:
  FILE* m_file = nullptr;
};
}  // end of namespace util
