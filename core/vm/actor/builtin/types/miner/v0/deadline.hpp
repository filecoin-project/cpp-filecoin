/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/deadline.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using types::miner::PartitionSectorMap;
  using types::miner::PoStPartition;
  using types::miner::PoStResult;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::Sectors;

  struct Deadline : types::miner::Deadline {
    outcome::result<PowerPair> recordFaults(
        const Sectors &sectors,
        SectorSize ssize,
        const QuantSpec &quant,
        ChainEpoch fault_expiration_epoch,
        const PartitionSectorMap &partition_sectors) override;

    outcome::result<std::tuple<PowerPair, PowerPair>> processDeadlineEnd(
        const QuantSpec &quant, ChainEpoch fault_expiration_epoch) override;

    outcome::result<PoStResult> recordProvenSectors(
        const Sectors &sectors,
        SectorSize ssize,
        const QuantSpec &quant,
        ChainEpoch fault_expiration,
        const std::vector<PoStPartition> &post_partitions) override;

    outcome::result<std::vector<SectorOnChainInfo>> rescheduleSectorExpirations(
        const Sectors &sectors,
        ChainEpoch expiration,
        const PartitionSectorMap &partition_sectors,
        SectorSize ssize,
        const QuantSpec &quant) override;

    outcome::result<void> validateState() const override;
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             partitions_posted,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power)

}  // namespace fc::vm::actor::builtin::v0::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v0::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
    }
  };
}  // namespace fc::cbor_blake
