/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/expiration.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using types::miner::PowerPair;
  using types::miner::SectorOnChainInfo;

  struct ExpirationQueue : public types::miner::ExpirationQueue {
    outcome::result<PowerPair> rescheduleAsFaults(
        ChainEpoch new_expiration,
        const std::vector<SectorOnChainInfo> &sectors,
        SectorSize ssize) override;

    outcome::result<void> rescheduleAllAsFaults(
        ChainEpoch fault_expiration) override;

    outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
    removeActiveSectors(const std::vector<SectorOnChainInfo> &sectors,
                        SectorSize ssize) override;
  };
}  // namespace fc::vm::actor::builtin::v0::miner
