/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/v2/partition.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::SectorSize;
  using runtime::Runtime;
  using types::miner::PowerPair;
  using types::miner::QuantSpec;
  using types::miner::SectorOnChainInfo;

  struct Partition : v2::miner::Partition {
    outcome::result<PowerPair> addSectors(
        Runtime &runtime,
        bool proven,
        const std::vector<SectorOnChainInfo> &sectors,
        SectorSize ssize,
        const QuantSpec &quant) override;
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

}  // namespace fc::vm::actor::builtin::v3::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::miner::Partition> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::miner::Partition &p,
                     const Visitor &visit) {
      visit(p.expirations_epochs);
      visit(p.early_terminated);
    }
  };
}  // namespace fc::cbor_blake
