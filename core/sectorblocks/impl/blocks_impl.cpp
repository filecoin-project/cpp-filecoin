/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"

namespace fc::sectorblocks {

  outcome::result<PieceAttributes> SectorBlocksImpl::addPiece(
      UnpaddedPieceSize size, const std::string &piece_data, DealInfo deal) {
    OUTCOME_TRY(
        piece_info,
        miner_->addPieceToAnySector(size, miner::PieceData(piece_data), deal));

    OUTCOME_TRY(writeRef(
        deal.deal_id, piece_info.sector, piece_info.offset, piece_info.size));

    return piece_info;
  }

  outcome::result<void> SectorBlocksImpl::writeRef(DealId deal_id,
                                                   SectorNumber sector,
                                                   PaddedPieceSize offset,
                                                   UnpaddedPieceSize size) {
    std::lock_guard lock(mutex_);
    storage_[deal_id].push_back(PieceLocation{
        .sector_number = sector,
        .offset = offset,
        .length = size,
    });
    return outcome::success();
  }

  outcome::result<std::vector<PieceLocation>> SectorBlocksImpl::getRefs(
      DealId deal_id) const {
    auto refs = storage_.find(deal_id);
    if (refs == storage_.end()) {
      return Error::kNotFound;
    }
    return refs->second;
  }

  std::shared_ptr<Miner> SectorBlocksImpl::getMiner() const {
    return miner_;
  }
}  // namespace fc::sectorblocks

OUTCOME_CPP_DEFINE_CATEGORY(fc::sectorblocks, Error, e) {
  using fc::sectorblocks::Error;
  switch (e) {
    case (Error::kNotFound):
      return "SectorBlocks: not found";
    default:
      return "SectorBlocks: unknown error";
  }
}
