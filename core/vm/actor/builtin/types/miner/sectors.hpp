/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/universal/universal_impl.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using types::Universal;

  constexpr size_t kSectorsBitwidth = 5;

  struct Sectors {
    adt::Array<Universal<SectorOnChainInfo>, kSectorsBitwidth> sectors;

    outcome::result<std::vector<Universal<SectorOnChainInfo>>> load(
        const RleBitset &sector_nos) const;

    outcome::result<void> store(
        const std::vector<Universal<SectorOnChainInfo>> &infos);

    outcome::result<std::vector<Universal<SectorOnChainInfo>>> loadForProof(
        const RleBitset &proven_sectors,
        const RleBitset &expected_faults) const;

    outcome::result<std::vector<Universal<SectorOnChainInfo>>>
    loadWithFaultMask(const RleBitset &sector_nums,
                      const RleBitset &faults,
                      SectorNumber faults_stand_in) const;

    outcome::result<Sectors> loadSectors() const;
  };

  inline CBOR2_DECODE(Sectors) {
    return s >> v.sectors;
  }

  inline CBOR2_ENCODE(Sectors) {
    return s << v.sectors;
  }

  outcome::result<std::vector<Universal<SectorOnChainInfo>>> selectSectors(
      const std::vector<Universal<SectorOnChainInfo>> &sectors,
      const RleBitset &field);

  // Methods are only for v0
  outcome::result<std::vector<Universal<SectorOnChainInfo>>>
  loadSectorInfosForProof(const Sectors &sectors,
                          const RleBitset &proven_sectors,
                          const RleBitset &expected_faults);

  outcome::result<std::vector<Universal<SectorOnChainInfo>>>
  loadSectorInfosWithFaultMask(const Sectors &sectors,
                               const RleBitset &sector_nums,
                               const RleBitset &faults,
                               SectorNumber faults_stand_in);

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
