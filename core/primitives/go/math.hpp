/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/big_int.hpp"

namespace fc::primitives::go {

  /// div with round-floor, like `big.Div` from go
  inline BigInt bigdiv(const BigInt &n, const BigInt &d) {
    if (!n.is_zero() && n.sign() != d.sign()) {
      return (n + 1) / d - 1;
    }
    return n / d;
  }

  /// mod with round-floor, like `big.Mod` from go
  inline BigInt bigmod(const BigInt &n, const BigInt &d) {
    const auto r = bigdiv(n, d);
    return n - r * d;
  }

}  // namespace fc::primitives::go

namespace fc {
  using primitives::go::bigdiv;
  using primitives::go::bigmod;
}  // namespace fc
