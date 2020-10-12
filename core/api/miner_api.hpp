/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_MINER_API_HPP
#define CPP_FILECOIN_CORE_API_MINER_API_HPP

#include "api/api.hpp"
#include "miner/storage_fsm/sealing.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::api {
  using mining::types::DealInfo;
  using mining::types::DealSchedule;
  using primitives::DealId;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SectorNumber;

  struct PieceLocation {
    SectorNumber sector_number;
    uint64_t offset;
    uint64_t length;
  };

  struct StorageMinerApi {
    // TODO(a.chernyshov): FIL-165 implement methods
  };

}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_MINER_API_HPP
