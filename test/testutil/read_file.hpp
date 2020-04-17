/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_READ_FILE_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_READ_FILE_HPP

#include <fstream>

#include <gtest/gtest.h>

#include "common/buffer.hpp"

auto readFile(const std::string &path) {
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  EXPECT_TRUE(file.good()) << "Cannot open file: " << path;
  fc::common::Buffer buffer;
  buffer.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
  return buffer;
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_READ_FILE_HPP
