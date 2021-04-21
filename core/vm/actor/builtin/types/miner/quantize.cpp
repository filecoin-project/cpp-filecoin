/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::types::miner {
  QuantSpec::QuantSpec(ChainEpoch unit, ChainEpoch offset)
      : unit(unit), offset(offset) {}

  ChainEpoch QuantSpec::quantizeUp(ChainEpoch e) const {
    const auto rounded_offset = offset % unit;

    const auto remainder = (e - rounded_offset) % unit;
    const auto quotient = (e - rounded_offset) / unit;

    if (remainder == 0) {
      return unit * quotient + rounded_offset;
    }

    if (e - rounded_offset < 0) {
      return unit * quotient + rounded_offset;
    }

    return unit * (quotient + 1) + rounded_offset;
  }

}  // namespace fc::vm::actor::builtin::types::miner
