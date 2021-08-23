/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::TokenAmount;

  struct VestingFunds {
    struct Fund {
      ChainEpoch epoch{};
      TokenAmount amount{};
    };
    std::vector<Fund> funds;

    TokenAmount unlockVestedFunds(ChainEpoch curr_epoch);

    void addLockedFunds(ChainEpoch curr_epoch,
                        const TokenAmount &vesting_sum,
                        ChainEpoch proving_period_start,
                        const VestSpec &spec);

    TokenAmount unlockUnvestedFunds(ChainEpoch curr_epoch,
                                    const TokenAmount &target);
  };
  CBOR_TUPLE(VestingFunds::Fund, epoch, amount)
  CBOR_TUPLE(VestingFunds, funds)

}  // namespace fc::vm::actor::builtin::types::miner
