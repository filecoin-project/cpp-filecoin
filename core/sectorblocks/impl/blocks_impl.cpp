/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/cbor_codec.hpp"

namespace fc::sectorblocks {
  using adt::UvarintKeyer;
  using codec::cbor::decode;
  using codec::cbor::encode;

  SectorBlocksImpl::SectorBlocksImpl(
      std::shared_ptr<Miner> miner,
      std::shared_ptr<DataStore> datastore)
      : miner_(std::move(miner)), storage_(std::move(datastore)) {}

  outcome::result<PieceAttributes> SectorBlocksImpl::addPiece(
      UnpaddedPieceSize size,
      const std::string &piece_data_path,
      DealInfo deal) {
    OUTCOME_TRY(piece_info,
                miner_->addPieceToAnySector(
                    size, miner::PieceData(piece_data_path), deal));

    OUTCOME_TRY(writeRef(
        deal.deal_id, piece_info.sector, piece_info.offset, piece_info.size));

    return piece_info;
  }

  outcome::result<void> SectorBlocksImpl::writeRef(DealId deal_id,
                                                   SectorNumber sector,
                                                   PaddedPieceSize offset,
                                                   UnpaddedPieceSize size) {
    std::lock_guard lock(mutex_);

    const Buffer key = Buffer(UvarintKeyer::encode(deal_id));
    std::vector<PieceLocation> new_data;
    const auto new_piece = PieceLocation{
        .sector_number = sector,
        .offset = offset,
        .length = size.padded(),
    };

    if (storage_->contains(key)) {
      OUTCOME_TRY(stored_data, storage_->get(key));
      OUTCOME_TRY(decoded_out, decode<std::vector<PieceLocation>>(stored_data));
      if (find(decoded_out.begin(), decoded_out.end(), new_piece)
          == decoded_out.end()) {
        decoded_out.push_back(new_piece);
        new_data = std::move(decoded_out);
      } else {
        return SectorBlocksError::kDealAlreadyExist;
      }
    } else {
      new_data = {new_piece};
    }

    OUTCOME_TRY(encoded_in, encode(new_data));
    return storage_->put(key, encoded_in);
  }

  outcome::result<std::vector<PieceLocation>> SectorBlocksImpl::getRefs(
      DealId deal_id) const {
    const Buffer key = Buffer(UvarintKeyer::encode(deal_id));
    if (storage_->contains(key)) {
      OUTCOME_TRY(stored_data, storage_->get(key));
      OUTCOME_TRY(decoded_out, decode<std::vector<PieceLocation>>(stored_data));
      return std::move(decoded_out);
    }
    return SectorBlocksError::kNotFoundDeal;
  }

  std::shared_ptr<Miner> SectorBlocksImpl::getMiner() const {
    return miner_;
  }
}  // namespace fc::sectorblocks

OUTCOME_CPP_DEFINE_CATEGORY(fc::sectorblocks, SectorBlocksError, e) {
  using fc::sectorblocks::SectorBlocksError;
  switch (e) {
    case (SectorBlocksError::kNotFoundDeal):
      return "SectorBlocks: not found";
    case(SectorBlocksError::kDealAlreadyExist):
      return "SectorBlocks: piece already exist in provided deal";
    default:
      return "SectorBlocks: unknown error";

  }
}
