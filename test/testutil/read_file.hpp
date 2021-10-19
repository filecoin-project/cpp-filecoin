/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>

#include "common/bytes.hpp"
#include "common/span.hpp"

auto readFile(const boost::filesystem::path &path) {
  std::ifstream file{path.c_str(), std::ios::binary | std::ios::ate};
  EXPECT_TRUE(file.good()) << "Cannot open file: " << path;
  fc::Bytes buffer;
  buffer.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(fc::common::span::string(buffer).data(), buffer.size());
  return buffer;
}
