/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/error_text.hpp"
#include "common/outcome.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::fvm {
  using primitives::BigInt;

  struct Ubig128 {
    uint64_t high{};
    uint64_t low{};

    static outcome::result<Ubig128> fromBig(const BigInt &big) {
      if (big.sign() < 0) {
        return ERROR_TEXT("Ubig128::fromBig negative");
      }
      if (big.is_zero()) {
        return Ubig128{};
      }
      if (msb(big) >= 128) {
        return ERROR_TEXT("Ubig128::fromBig too big");
      }
      struct Iterator {
        Ubig128 u;
        size_t i{0};

        auto &operator*() {
          return *this;
        }
        void operator=(uint64_t v) {
          if (i == 0) {
            u.low = v;
          } else if (i == 1) {
            u.high = v;
          }
        }
        void operator++() {
          ++i;
        }
      };
      const auto it{export_bits(big, Iterator{}, 64, false)};
      return it.u;
    }

    BigInt big() const {
      BigInt big;
      std::initializer_list<uint64_t> limbs{low, high};
      import_bits(big, limbs.begin(), limbs.end(), 64, false);
      return big;
    }
  };
}  // namespace fc::vm::fvm
