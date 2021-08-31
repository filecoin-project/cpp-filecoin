/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <iosfwd>

#include "common/buffer.hpp"
#include "common/outcome2.hpp"
#include "common/span.hpp"

namespace fc::common {
  using MappedFile = boost::iostreams::mapped_file_source;

  Outcome<std::pair<MappedFile, BytesIn>> mapFile(const std::string &path);

  Outcome<Buffer> readFile(const boost::filesystem::path &path);

  Outcome<void> writeFile(const boost::filesystem::path &path, BytesIn input);

  /** returns true on success */
  inline bool read(std::istream &is, gsl::span<uint8_t> bytes) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return is.read(reinterpret_cast<char *>(bytes.data()), bytes.size()).good();
  }

  /** returns true on success */
  template <typename T>
  inline bool read(std::istream &is, gsl::span<T> values) {
    return read(is, span::cast<uint8_t>(values));
  }

  template <typename T>
  inline bool readStruct(std::istream &is, T &value) {
    return read(is, gsl::make_span(&value, 1));
  }

  /** returns true on success */
  inline bool write(std::ostream &is, BytesIn bytes) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return is.write(reinterpret_cast<const char *>(bytes.data()), bytes.size())
        .good();
  }

  /** returns true on success */
  template <typename T>
  inline bool write(std::ostream &is, gsl::span<T> values) {
    return write(is, span::cast<const uint8_t>(values));
  }

  template <typename T>
  inline bool writeStruct(std::ostream &is, const T &value) {
    return write(is, gsl::make_span(&value, 1));
  }
}  // namespace fc::common
