/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MINER_MINER_HPP
#define CPP_FILECOIN_MINER_MINER_HPP

#include "miner/storage_fsm/types.hpp"
#include "primitives/types.hpp"

namespace fc::miner {
  using mining::types::DealInfo;
  using mining::types::PieceAttributes;
  using mining::types::SectorInfo;
  using primitives::SectorNumber;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;

  class Miner {
   public:
    virtual ~Miner() = default;

    virtual outcome::result<void> run() = 0;

    virtual void stop() = 0;

    virtual outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber sector_id) const = 0;

    virtual outcome::result<PieceAttributes> addPieceToAnySector(
        UnpaddedPieceSize size, const PieceData &piece_data, DealInfo deal) = 0;

    virtual Address getAddress() const = 0;
  };

  enum class MinerError {
    kWorkerNotFound = 1,
  };

}  // namespace fc::miner

OUTCOME_HPP_DECLARE_ERROR(fc::miner, MinerError);

#endif  // CPP_FILECOIN_MINER_MINER_HPP
