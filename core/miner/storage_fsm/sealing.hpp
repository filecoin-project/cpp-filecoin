/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/piece/piece_data.hpp"

namespace fc::mining {
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using proofs::PieceData;
  using types::DealInfo;
  using types::PieceLocation;
  using types::SectorInfo;

  struct Config {
    // A cap of deals, 0 = no limit
    uint64_t max_wait_deals_sectors = 0;

    // includes failed, 0 = no limit
    uint64_t max_sealing_sectors = 0;

    // includes failed, 0 = no limit
    uint64_t max_sealing_sectors_for_deals = 0;

    std::chrono::milliseconds wait_deals_delay{};  // in milliseconds

    bool batch_pre_commits = false;
  };

  class Sealing {
   public:
    virtual ~Sealing() = default;

    virtual outcome::result<PieceLocation> addPieceToAnySector(
        const UnpaddedPieceSize &size,
        PieceData piece_data,
        const DealInfo &deal) = 0;

    virtual outcome::result<void> remove(SectorNumber sector_id) = 0;

    virtual Address getAddress() const = 0;

    virtual std::vector<std::shared_ptr<const SectorInfo>> getListSectors()
        const = 0;

    virtual outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber id) const = 0;

    virtual outcome::result<void> forceSectorState(SectorNumber id,
                                                   SealingState state) = 0;

    virtual outcome::result<void> markForUpgrade(SectorNumber id) = 0;

    virtual bool isMarkedForUpgrade(SectorNumber id) = 0;

    virtual outcome::result<void> startPacking(SectorNumber id) = 0;

    virtual outcome::result<void> pledgeSector() = 0;
  };

  enum class SealingError {
    kPieceNotFit = 1,
    kCannotAllocatePiece,
    kCannotFindSector,
    kAlreadyUpgradeMarked,
    kNotProvingState,
    kUpgradeSeveralPieces,
    kUpgradeWithDeal,
    kTooManySectors,
    kNoFaultMessage,
    kFailSubmit,
    kSectorAllocatedError,
    kNotPublishedDeal,
  };
}  // namespace fc::mining

OUTCOME_HPP_DECLARE_ERROR(fc::mining, SealingError);
