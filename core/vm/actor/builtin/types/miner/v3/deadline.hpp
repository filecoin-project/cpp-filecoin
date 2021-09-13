/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/v2/deadline.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using runtime::Runtime;
  using types::miner::PartitionSectorMap;
  using types::miner::PoStPartition;
  using types::miner::PoStResult;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;
  using types::miner::Sectors;

  struct Deadline : v2::miner::Deadline {
    outcome::result<std::tuple<PowerPair, PowerPair>> processDeadlineEnd(
        Runtime &runtime,
        const QuantSpec &quant,
        ChainEpoch fault_expiration_epoch) override;

    outcome::result<PoStResult> recordProvenSectors(
        Runtime &runtime,
        const Sectors &sectors,
        SectorSize ssize,
        const QuantSpec &quant,
        ChainEpoch fault_expiration,
        const std::vector<PoStPartition> &post_partitions) override;
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             partitions_posted,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power,
             optimistic_post_submissions,
             partitions_snapshot,
             optimistic_post_submissions_snapshot)

}  // namespace fc::vm::actor::builtin::v3::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
      visit(d.optimistic_post_submissions);
      visit(d.partitions_snapshot);
      visit(d.optimistic_post_submissions_snapshot);
    }
  };
}  // namespace fc::cbor_blake
