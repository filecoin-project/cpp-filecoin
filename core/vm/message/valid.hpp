/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/market/policy.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {
  inline bool validForBlockInclusion(const UnsignedMessage &msg,
                                     NetworkVersion network,
                                     GasAmount gas) {
    if (msg.version != kMessageVersion) {
      return false;
    }
    if (network >= NetworkVersion::kVersion7) {
      static const auto kZero{Address::makeBls({})};
      if (msg.to == kZero) {
        return false;
      }
    }
    if (msg.value < 0) {
      return false;
    }
    if (msg.value > actor::builtin::types::market::kTotalFilecoin) {
      return false;
    }
    if (msg.gas_fee_cap < 0) {
      return false;
    }
    if (msg.gas_premium > msg.gas_fee_cap) {
      return false;
    }
    if (msg.gas_limit > kBlockGasLimit) {
      return false;
    }
    if (msg.gas_limit < gas) {
      return false;
    }
    return true;
  }
}  // namespace fc::vm::message
