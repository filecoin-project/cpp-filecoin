/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;

  struct QuantSpec {
    ChainEpoch unit{};
    ChainEpoch offset{};

    QuantSpec() = default;
    constexpr QuantSpec(ChainEpoch unit, ChainEpoch offset)
        : unit{unit}, offset{offset} {}

    // Rounds e to the nearest exact multiple of the quantization unit offset by
    // offsetSeed % unit, rounding up.
    // This function is equivalent to `unit * ceil(e - (offsetSeed % unit) /
    // unit) + (offsetSeed % unit)` with the variables/operations are over real
    // numbers instead of ints. Precondition: unit >= 0 else behaviour is
    // undefined
    ChainEpoch quantizeUp(ChainEpoch e) const;

    // QuantizeDown == QuantizeUp if e is a fixed point of QuantizeUp
    ChainEpoch quantizeDown(ChainEpoch e) const;
  };

  constexpr QuantSpec kNoQuantization{1, 0};
}  // namespace fc::vm::actor::builtin::types::miner
