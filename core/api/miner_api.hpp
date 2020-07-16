/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_MINER_API_HPP
#define CPP_FILECOIN_CORE_API_MINER_API_HPP

#include "api/api.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/sealing/types.hpp"

namespace fc::api {
  using primitives::DealId;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SectorNumber;
  using sector_storage::sealing::DealInfo;
  using sector_storage::sealing::DealSchedule;

  struct PieceLocation {
    SectorNumber sector_number;
    uint64_t offset;
    uint64_t length;
  };

  // TODO(a.chernyshov): FIL-165 implement methods
  struct MinerApi {
    /**
     * Add piece to sector blocks
     * @param piece_size - unpadded size of added piece
     * @param data - piece data
     * @param deal_info - info about commited storage deal
     */
    API_METHOD(AddPiece,
               void,
               const UnpaddedPieceSize &,
               const Buffer &,
               const DealInfo &)

    /**
     * Get location for the data for storage deal
     */
    API_METHOD(LocatePieceForDealWithinSector,
               PieceLocation,
               const DealId &,
               const TipsetKey &)
  };

}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_MINER_API_HPP
