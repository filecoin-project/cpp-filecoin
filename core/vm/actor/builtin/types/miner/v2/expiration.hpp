/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/expiration.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using types::Universal;
  using types::miner::ExpirationSet;
  using types::miner::PowerPair;
  using types::miner::SectorExpirationSet;
  using types::miner::SectorOnChainInfo;

  struct ExpirationQueue : public types::miner::ExpirationQueue {
    outcome::result<PowerPair> rescheduleAsFaults(
        ChainEpoch new_expiration,
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize) override;

    outcome::result<void> rescheduleAllAsFaults(
        ChainEpoch fault_expiration) override;

    outcome::result<std::tuple<RleBitset, PowerPair, TokenAmount>>
    removeActiveSectors(
        const std::vector<Universal<SectorOnChainInfo>> &sectors,
        SectorSize ssize) override;

   private:
    outcome::result<std::vector<SectorExpirationSet>> findSectorsByExpiration(
        SectorSize ssize,
        const std::vector<Universal<SectorOnChainInfo>> &sectors);

    std::tuple<SectorExpirationSet, RleBitset> groupExpirationSet(
        SectorSize ssize,
        const std::map<SectorNumber, Universal<SectorOnChainInfo>> &sectors,
        RleBitset &include_set,
        const ExpirationSet &es,
        ChainEpoch expiration);
  };
  CBOR_NON(ExpirationQueue);
}  // namespace fc::vm::actor::builtin::v2::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v2::miner::ExpirationQueue> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::miner::ExpirationQueue &p,
                     const Visitor &visit) {
      visit(p.queue);
    }
  };
}  // namespace fc::cbor_blake
