/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/file.hpp"

#include <fstream>

#include "common/span.hpp"

namespace fc::common {
  Outcome<std::pair<MappedFile, BytesIn>> mapFile(const std::string &path) {
    MappedFile file{path};
    if (!file.is_open()) {
      return {};
    }
    auto input{
        common::span::cbytes(std::string_view{file.data(), file.size()})};
    return std::make_pair(std::move(file), input);
  }

  Outcome<Buffer> readFile(std::string_view path) {
    std::ifstream file{path.data(), std::ios::binary | std::ios::ate};
    if (file.good()) {
      Buffer buffer;
      buffer.resize(file.tellg());
      file.seekg(0, std::ios::beg);
      file.read(common::span::string(buffer).data(), buffer.size());
      return buffer;
    }
    return {};
  }

  outcome::result<void> writeFile(std::string_view path, BytesIn input) {
    // TODO: mkdir
    std::ofstream file{path.data(), std::ios::binary};
    if (file.good()) {
      file.write(span::bytestr(input.data()), input.size());
      return outcome::success();
    }
    return OutcomeError::kDefault;
  }
}  // namespace fc::common
