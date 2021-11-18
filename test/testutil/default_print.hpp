/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest-printers.h>
#include <boost/optional/optional_fwd.hpp>

namespace fc::crypto::signature {
  struct Signature;
  inline void PrintTo(const Signature &v, ::std::ostream *os) {
    *os << ::testing::PrintToString(v);
  }
}  // namespace fc::crypto::signature

namespace boost {
  template <typename T>
  void PrintTo(const optional<T> &v, ::std::ostream *os) {
    *os << ::testing::PrintToString(v);
  }
}  // namespace boost
