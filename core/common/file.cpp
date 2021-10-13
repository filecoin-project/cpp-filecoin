/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/file.hpp"

#include <boost/filesystem/operations.hpp>
#include <fstream>

#include "common/error_text.hpp"
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

  Outcome<Bytes> readFile(const boost::filesystem::path &path) {
    std::ifstream file{path.c_str(), std::ios::binary | std::ios::ate};
    if (file.good()) {
      Bytes result;
      result.resize(file.tellg());
      file.seekg(0, std::ios::beg);
      file.read(common::span::string(result).data(),
                static_cast<ptrdiff_t>(result.size()));
      return result;
    }
    return {};
  }

  Outcome<void> writeFile(const boost::filesystem::path &path, BytesIn input) {
    if (path.has_parent_path()) {
      boost::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file{path.c_str(), std::ios::binary};
    if (file.write(span::bytestr(input.data()), input.size()).good()) {
      return outcome::success();
    }
    return ERROR_TEXT("writeFile: error");
  }
}  // namespace fc::common
