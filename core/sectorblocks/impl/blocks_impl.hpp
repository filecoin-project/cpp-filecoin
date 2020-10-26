/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTORBLOCKS_IMPL_BLOCKS_IMPL_HPP
#define CPP_FILECOIN_CORE_SECTORBLOCKS_IMPL_BLOCKS_IMPL_HPP

#include "sectorblocks/blocks.hpp"

namespace fc::sectorblocks {
  using primitives::SectorNumber;
  using primitives::piece::PaddedPieceSize;

  class SectorBlocksImpl : public SectorBlocks {
   public:
    SectorBlocksImpl(std::shared_ptr<Miner> miner);

    outcome::result<PieceAttributes> addPiece(UnpaddedPieceSize size,
                                              const std::string &piece_data,
                                              DealInfo deal) override;

    outcome::result<std::vector<PieceLocation>> getRefs(
        DealId deal_id) const override;

    std::shared_ptr<Miner> getMiner() const override;

   private:
    outcome::result<void> writeRef(DealId deal_id,
                                   SectorNumber sector,
                                   PaddedPieceSize offset,
                                   UnpaddedPieceSize size);

    std::shared_ptr<Miner> miner_;

    std::mutex mutex_;
    std::map<DealId, std::vector<PieceLocation>>
        storage_;  // TODO: change to DataStore
  };

}  // namespace fc::sectorblocks

#endif  // CPP_FILECOIN_CORE_SECTORBLOCKS_IMPL_BLOCKS_IMPL_HPP
