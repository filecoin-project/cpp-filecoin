/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTORBLOCKS_BLOCKS_HPP
#define CPP_FILECOIN_CORE_SECTORBLOCKS_BLOCKS_HPP

#include "api/storage_api.hpp"
#include "miner/miner.hpp"
#include "primitives/types.hpp"

namespace fc::sectorblocks {
  using api::DealInfo;
  using api::PieceLocation;
  using miner::Miner;
  using mining::types::PieceAttributes;
  using primitives::DealId;
  using primitives::piece::UnpaddedPieceSize;

  class SectorBlocks {
   public:
    virtual ~SectorBlocks() = default;

    virtual outcome::result<PieceAttributes> addPiece(
        UnpaddedPieceSize size,
        const std::string &piece_data_path,
        DealInfo deal) = 0;

    virtual outcome::result<std::vector<PieceLocation>> getRefs(
        DealId deal_id) const = 0;

    virtual std::shared_ptr<Miner> getMiner() const = 0;
  };

  enum class SectorBlocksError {
    kNotFoundDeal = 1,
  };

}  // namespace fc::sectorblocks

OUTCOME_HPP_DECLARE_ERROR(fc::sectorblocks, SectorBlocksError);

#endif  // CPP_FILECOIN_CORE_SECTORBLOCKS_BLOCKS_HPP
