/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/sectors.hpp"

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {
  outcome::result<std::vector<SectorOnChainInfo>> Sectors::load(
      const RleBitset &sector_nos) const {
    std::vector<SectorOnChainInfo> sector_infos;

    for (const auto i : sector_nos) {
      OUTCOME_TRY(sector, sectors.get(i));
      sector_infos.push_back(sector);
    }

    return sector_infos;
  }

  outcome::result<void> Sectors::store(
      const std::vector<SectorOnChainInfo> &infos) {
    for (const auto &info : infos) {
      if (info.sector > kMaxSectorNumber) {
        return ERROR_TEXT("sector number is out of range");
      }
      OUTCOME_TRY(sectors.set(info.sector, info));
    }
    return outcome::success();
  }

  outcome::result<std::vector<SectorOnChainInfo>> Sectors::loadForProof(
      const RleBitset &proven_sectors, const RleBitset &expected_faults) const {
    const RleBitset non_faults = proven_sectors - expected_faults;

    if (non_faults.empty()) {
      return std::vector<SectorOnChainInfo>{};
    }

    const auto &good_sector = *non_faults.begin();

    return loadWithFaultMask(proven_sectors, expected_faults, good_sector);
  }

  outcome::result<std::vector<SectorOnChainInfo>> Sectors::loadWithFaultMask(
      const RleBitset &sector_nums,
      const RleBitset &faults,
      SectorNumber faults_stand_in) const {
    OUTCOME_TRY(stand_in_info, sectors.get(faults_stand_in));

    std::vector<SectorOnChainInfo> sector_infos;
    for (const auto i : sector_nums) {
      auto sector = stand_in_info;
      if (!faults.has(i)) {
        OUTCOME_TRY(sector_on_chain, sectors.get(i));
        sector = sector_on_chain;
      }
      sector_infos.push_back(sector);
    }

    return sector_infos;
  }

  outcome::result<Sectors> Sectors::loadSectors() const {
    const auto sectors_copy = *this;

    // assert that root is not loaded
    sectors_copy.sectors.amt.cid();

    // Lotus gas conformance
    OUTCOME_TRY(sectors_copy.sectors.amt.loadRoot());

    return sectors_copy;
  }

  outcome::result<std::vector<SectorOnChainInfo>> selectSectors(
      const std::vector<SectorOnChainInfo> &sectors, const RleBitset &field) {
    auto to_include = field;

    std::vector<SectorOnChainInfo> included;
    for (const auto &sector : sectors) {
      if (!to_include.has(sector.sector)) {
        continue;
      }
      included.push_back(sector);
      to_include.erase(sector.sector);
    }
    if (!to_include.empty()) {
      return ERROR_TEXT("failed to find expected sectors");
    }

    return included;
  }

  outcome::result<std::vector<SectorOnChainInfo>> loadSectorInfosForProof(
      const Sectors &sectors,
      const RleBitset &proven_sectors,
      const RleBitset &expected_faults) {
    const RleBitset non_faults = proven_sectors - expected_faults;

    if (non_faults.empty()) {
      return std::vector<SectorOnChainInfo>{};
    }

    const auto &good_sector = *non_faults.begin();

    return loadSectorInfosWithFaultMask(
        sectors, proven_sectors, expected_faults, good_sector);
  }

  outcome::result<std::vector<SectorOnChainInfo>> loadSectorInfosWithFaultMask(
      const Sectors &sectors,
      const RleBitset &sector_nums,
      const RleBitset &faults,
      SectorNumber faults_stand_in) {
    OUTCOME_TRY(sectors_arr, sectors.loadSectors());
    return sectors_arr.loadWithFaultMask(sector_nums, faults, faults_stand_in);
  }

}  // namespace fc::vm::actor::builtin::types::miner
