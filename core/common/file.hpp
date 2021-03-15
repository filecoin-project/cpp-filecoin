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

  outcome::result<void> writeFile(const boost::filesystem::path &path,
                                  BytesIn input,
                                  bool tmp = false);

  /** returns true on success */
  inline bool read(std::istream &is, gsl::span<uint8_t> bytes) {
    return is.read((char *)bytes.data(), bytes.size()).good();
  }

  /** returns true on success */
  template <typename T>
  inline bool read(std::istream &is, gsl::span<T> values) {
    return read(is, span::cast<uint8_t>(values));
  }
}  // namespace fc::common
