/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP

#include "common/blob.hpp"
#include "common/hexutil.hpp"

inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s) {
  return fc::common::unhex(std::string_view(c, s)).value();
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_LITERALS_HPP
