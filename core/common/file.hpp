/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "common/buffer.hpp"
#include "common/outcome2.hpp"

namespace fc::common {
  using MappedFile = boost::iostreams::mapped_file_source;

  Outcome<std::pair<MappedFile, BytesIn>> mapFile(const std::string &path);

  Outcome<Buffer> readFile(const boost::filesystem::path &path);

  outcome::result<void> writeFile(const boost::filesystem::path &path,
                                  BytesIn input);
}  // namespace fc::common
