/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sectorblocks/impl/blocks_impl.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/cbor_codec.hpp"
#include "storage/leveldb/prefix.hpp"

namespace fc::sectorblocks {
  using adt::UvarintKeyer;
  using codec::cbor::decode;
  using codec::cbor::encode;
  using storage::MapPrefix;

  SectorBlocksImpl::SectorBlocksImpl(
      std::shared_ptr<Miner> miner, const std::shared_ptr<DataStore> &datastore)
      : miner_{miner} {
    store_ = std::make_shared<storage::MapPrefix>("sealedblocks/", datastore);
  }

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

    Buffer key = Buffer(UvarintKeyer::encode(deal_id));
    auto new_piece = PieceLocation{
        .sector_number = sector,
        .offset = offset,
        .length = size.padded(),
    };

    OUTCOME_TRY(stored_data, store_->get(key));
    if (stored_data.size() != 0) {
      OUTCOME_TRY(decoded_out, decode<std::vector<PieceLocation>>(stored_data));
      bool flag = false;
      for (const auto &piece : decoded_out) {
        if (piece == new_piece) {
          flag = true;
        }
      }
      if (!flag) {
        decoded_out.push_back(new_piece);
        OUTCOME_TRY(encoded_in, encode(decoded_out));
        OUTCOME_TRY(store_->put(key, encoded_in));
      }
    } else {
      std::vector<PieceLocation> new_data = {new_piece};
      OUTCOME_TRY(encoded_in, encode(new_data));
      OUTCOME_TRY(store_->put(key, encoded_in));
    }
    return outcome::success();
  }

  outcome::result<std::vector<PieceLocation>> SectorBlocksImpl::getRefs(
      DealId deal_id) const {
    // auto refs = storage_.find(deal_id);
    Buffer key = Buffer(UvarintKeyer::encode(deal_id));
    OUTCOME_TRY(stored_data, store_->get(key));
    if (stored_data.size() == 0) {
      return SectorBlocksError::kNotFoundDeal;
    }
    OUTCOME_TRY(decoded_out, decode<std::vector<PieceLocation>>(stored_data));
    return std::move(decoded_out);
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
    default:
      return "SectorBlocks: unknown error";
  }
}
