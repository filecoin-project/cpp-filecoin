/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/file.hpp"

#include <fstream>

#include "common/span.hpp"

namespace fc::common {
  Outcome<Buffer> readFile(std::string_view path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (file.good()) {
      Buffer buffer;
      buffer.resize(file.tellg());
      file.seekg(0, std::ios::beg);
      file.read(common::span::string(buffer).data(), buffer.size());
      return buffer;
    }
    return {};
  }
}  // namespace fc::common
