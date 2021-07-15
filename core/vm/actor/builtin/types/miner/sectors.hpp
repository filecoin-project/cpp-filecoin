/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "common/outcome.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;
  using primitives::SectorNumber;

  struct Sectors {
    adt::Array<SectorOnChainInfo> sectors;

    outcome::result<std::vector<SectorOnChainInfo>> load(
        const RleBitset &sector_nos) const;

    outcome::result<void> store(const std::vector<SectorOnChainInfo> &infos);

    outcome::result<std::vector<SectorOnChainInfo>> loadForProof(
        const RleBitset &proven_sectors,
        const RleBitset &expected_faults) const;

    outcome::result<std::vector<SectorOnChainInfo>> loadWithFaultMask(
        const RleBitset &sector_nums,
        const RleBitset &faults,
        SectorNumber faults_stand_in) const;
  };

  outcome::result<std::vector<SectorOnChainInfo>> selectSectors(
      const std::vector<SectorOnChainInfo> &sectors, const RleBitset &field);

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::types::miner::Sectors> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::Sectors &p,
                     const Visitor &visit) {
      visit(p.sectors);
    }
  };
}  // namespace fc::cbor_blake
