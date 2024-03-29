/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sectorblocks/blocks.hpp"
#include "storage/buffer_map.hpp"

namespace fc::sectorblocks {
  using primitives::SectorNumber;
  using primitives::piece::PaddedPieceSize;
  using DataStore = storage::PersistentBufferMap;

  class SectorBlocksImpl : public SectorBlocks {
   public:
    SectorBlocksImpl(std::shared_ptr<Miner> miner,
                     std::shared_ptr<DataStore> datastore);

    outcome::result<PieceLocation> addPiece(UnpaddedPieceSize size,
                                            const std::string &piece_data_path,
                                            DealInfo deal) override;

    outcome::result<std::vector<PieceLocation>> getRefs(
        DealId deal_id) const override;

    std::shared_ptr<Miner> getMiner() const override;

   private:
    outcome::result<void> writeRef(DealId deal_id,
                                   SectorNumber sector,
                                   PaddedPieceSize offset,
                                   PaddedPieceSize size);

    // TODO(@Elestrias):[FIL-423] Make deletion of expired deal associations;

    std::shared_ptr<Miner> miner_;

    std::mutex mutex_;
    std::shared_ptr<DataStore> storage_;
  };

}  // namespace fc::sectorblocks
