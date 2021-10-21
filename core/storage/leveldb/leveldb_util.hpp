/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <leveldb/status.h>
#include <gsl/span>
#include "common/bytes.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "storage/leveldb/leveldb_error.hpp"

namespace fc::storage {

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s) {
    if (s.IsNotFound()) {
      return LevelDBError::kNotFound;
    }

    if (s.IsIOError()) {
      return LevelDBError::kIOError;
    }

    if (s.IsInvalidArgument()) {
      return LevelDBError::kInvalidArgument;
    }

    if (s.IsCorruption()) {
      return LevelDBError::kCorruption;
    }

    if (s.IsNotSupportedError()) {
      return LevelDBError::kNotSupported;
    }

    return LevelDBError::kUnknown;
  }

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s,
                                            const common::Logger &logger) {
    // "not found" is not an error of leveldb
    if (!s.IsNotFound()) {
      logger->error(s.ToString());
    }
    return error_as_result<T>(s);
  }

  inline leveldb::Slice make_slice(const BytesIn &buf) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const char *>(buf.data());
    size_t n = buf.size();
    return leveldb::Slice{ptr, n};
  }

  inline gsl::span<const uint8_t> make_span(const leveldb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const uint8_t *>(s.data());
    return gsl::make_span(ptr, s.size());
  }

  inline Bytes make_buffer(const leveldb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const uint8_t *>(s.data());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Bytes(ptr, ptr + s.size());
  }

}  // namespace fc::storage
