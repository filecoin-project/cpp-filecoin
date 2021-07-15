/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/partition.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorSize;
  using runtime::Runtime;
  using types::miner::ExpirationSet;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::Sectors;

  struct Partition : types::miner::Partition {
    RleBitset activeSectors() const override;

    PowerPair activePower() const override;

    outcome::result<PowerPair> addSectors(
        Runtime &runtime,
        bool proven,
        const std::vector<SectorOnChainInfo> &sectors,
        SectorSize ssize,
        const QuantSpec &quant) override;

    outcome::result<std::tuple<PowerPair, PowerPair>> addFaults(
        Runtime &runtime,
        const RleBitset &sector_nos,
        const std::vector<SectorOnChainInfo> &sectors,
        ChainEpoch fault_expiration,
        SectorSize ssize,
        const QuantSpec &quant) override;

    outcome::result<ExpirationSet> terminateSectors(
        Runtime &runtime,
        const Sectors &sectors,
        ChainEpoch epoch,
        const RleBitset &sector_nos,
        SectorSize ssize,
        const QuantSpec &quant) override;

    outcome::result<ExpirationSet> popExpiredSectors(
        Runtime &runtime, ChainEpoch until, const QuantSpec &quant) override;

    outcome::result<void> validateState() const override;
  };

  CBOR_TUPLE(Partition,
             sectors,
             unproven,
             faults,
             recoveries,
             terminated,
             expirations_epochs,
             early_terminated,
             live_power,
             unproven_power,
             faulty_power,
             recovering_power)

}  // namespace fc::vm::actor::builtin::v2::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v2::miner::Partition> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v2::miner::Partition &p,
                     const Visitor &visit) {
      visit(p.expirations_epochs);
      visit(p.early_terminated);
    }
  };
}  // namespace fc::cbor_blake
