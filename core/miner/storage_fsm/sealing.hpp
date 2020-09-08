/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP
#define CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP

#include "common/outcome.hpp"
#include "miner/storage_fsm/sealing_states.hpp"
#include "miner/storage_fsm/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/piece/piece_data.hpp"

namespace fc::mining {
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::piece::UnpaddedPieceSize;
  using types::DealInfo;
  using types::PieceAttributes;
  using types::SectorInfo;

  class Sealing {
   public:
    virtual ~Sealing() = default;

    virtual outcome::result<void> run() = 0;

    virtual void stop() = 0;

    virtual outcome::result<PieceAttributes> addPieceToAnySector(
        UnpaddedPieceSize size, const PieceData &piece_data, DealInfo deal) = 0;

    virtual outcome::result<void> remove(SectorNumber sector_id) = 0;

    virtual Address getAddress() const = 0;

    virtual std::vector<SectorNumber> getListSectors() const = 0;

    virtual outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber id) const = 0;

    virtual outcome::result<void> forceSectorState(SectorNumber id,
                                                   SealingState state) = 0;

    virtual outcome::result<void> markForUpgrade(SectorNumber id) = 0;

    virtual bool isMarkedForUpgrade(SectorNumber id) = 0;

    virtual outcome::result<void> startPacking(SectorNumber id) = 0;

    virtual outcome::result<void> pledgeSector() = 0;
  };
}  // namespace fc::mining

#endif  // CPP_FILECOIN_CORE_MINER_STORAGE_FSM_SEALING_HPP
