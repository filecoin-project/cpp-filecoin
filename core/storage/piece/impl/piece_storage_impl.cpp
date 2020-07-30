/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/piece/impl/piece_storage_impl.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::storage::piece {
  bool DealInfo::operator==(const DealInfo &other) const {
    return deal_id == other.deal_id && sector_id == other.sector_id
           && offset == other.offset && length == other.length;
  }

  bool PieceInfo::operator==(const PieceInfo &other) const {
    return piece_cid == other.piece_cid && deals == other.deals;
  }

  bool PayloadLocation::operator==(const PayloadLocation &other) const {
    return relative_offset == other.relative_offset
           && block_size == other.block_size;
  }

  bool PayloadBlockInfo::operator==(const PayloadBlockInfo &other) const {
    return parent_piece == other.parent_piece
           && block_location == other.block_location;
  }

  PieceStorageImpl::PieceStorageImpl(
      std::shared_ptr<PersistentMap> storage_backend)
      : storage_{std::move(storage_backend)} {}

  outcome::result<void> PieceStorageImpl::addDealForPiece(
      const CID &piece_cid, const DealInfo &deal_info) {
    OUTCOME_TRY(storage_key,
                PieceStorageImpl::makeKey(kPiecePrefix, piece_cid));
    PieceInfo piece_info{.piece_cid = piece_cid, .deals = {}};
    if (storage_->contains(storage_key)) {
      OUTCOME_TRYA(piece_info, getPieceInfo(piece_cid));
    }
    piece_info.deals.push_back(deal_info);
    OUTCOME_TRY(value, codec::cbor::encode(piece_info));
    OUTCOME_TRY(storage_->put(storage_key, value));
    return outcome::success();
  }

  outcome::result<PieceInfo> PieceStorageImpl::getPieceInfo(
      const CID &piece_cid) const {
    OUTCOME_TRY(storage_key,
                PieceStorageImpl::makeKey(kPiecePrefix, piece_cid));
    if (!storage_->contains(storage_key)) {
      return PieceStorageError::kPieceNotFound;
    }
    OUTCOME_TRY(value, storage_->get(storage_key));
    OUTCOME_TRY(piece_info, codec::cbor::decode<PieceInfo>(value));
    return std::move(piece_info);
  }

  outcome::result<PayloadInfo> PieceStorageImpl::getPayloadInfo(
      const CID &payload_cid) const {
    OUTCOME_TRY(storage_key,
                PieceStorageImpl::makeKey(kLocationPrefix, payload_cid));
    if (!storage_->contains(storage_key)) {
      return PieceStorageError::kPayloadNotFound;
    }
    OUTCOME_TRY(buffer, storage_->get(storage_key));
    OUTCOME_TRY(payload_info, codec::cbor::decode<PayloadInfo>(buffer));
    return std::move(payload_info);
  }

  outcome::result<void> PieceStorageImpl::addPayloadLocations(
      const CID &parent_piece, std::map<CID, PayloadLocation> locations) {
    for (auto &&[payload_cid, location] : locations) {
      OUTCOME_TRY(storage_key,
                  PieceStorageImpl::makeKey(kLocationPrefix, payload_cid));
      PayloadInfo payload_info{.cid = payload_cid, .piece_block_locations = {}};
      if (storage_->contains(storage_key)) {
        OUTCOME_TRYA(payload_info, getPayloadInfo(payload_cid));
      }
      PayloadBlockInfo payload_block_info{.parent_piece = parent_piece,
                                          .block_location = location};
      payload_info.piece_block_locations.push_back(payload_block_info);
      OUTCOME_TRY(value, codec::cbor::encode(payload_info));
      OUTCOME_TRY(storage_->put(storage_key, value));
    }
    return outcome::success();
  }

  outcome::result<PieceInfo> PieceStorageImpl::getPieceInfoFromCid(
      const CID &payload_cid, const boost::optional<CID> &piece_cid) const {
    OUTCOME_TRY(cid_info, getPayloadInfo(payload_cid));
    for (auto &&block_location : cid_info.piece_block_locations) {
      OUTCOME_TRY(piece_info, getPieceInfo(block_location.parent_piece));
      if (!piece_cid.has_value() || piece_info.piece_cid == piece_cid.get()) {
        return std::move(piece_info);
      }
    }
    return PieceStorageError::kPieceNotFound;
  }

  outcome::result<bool> PieceStorageImpl::hasPieceInfo(
      CID payload_cid, const boost::optional<CID> &piece_cid) const {
    auto piece_info_res = getPieceInfoFromCid(payload_cid, piece_cid);
    if (piece_info_res.has_error()
        && piece_info_res.error() == PieceStorageError::kPieceNotFound) {
      return false;
    }
    return !piece_info_res.value().deals.empty();
  }

  outcome::result<uint64_t> PieceStorageImpl::getPieceSize(
      CID payload_cid, const boost::optional<CID> &piece_cid) const {
    OUTCOME_TRY(piece_info, getPieceInfoFromCid(payload_cid, piece_cid));
    if (!piece_info.deals.empty()) {
      return piece_info.deals.front().length;
    }
    return PieceStorageError::kPieceNotFound;
  }

  outcome::result<Buffer> PieceStorageImpl::makeKey(const std::string &prefix,
                                                    const CID &cid) {
    OUTCOME_TRY(cid_key, cid.toString());
    std::vector<uint8_t> key_bytes{std::move_iterator(prefix.begin()),
                                   std::move_iterator(prefix.end())};
    key_bytes.insert(key_bytes.end(),
                     std::move_iterator(cid_key.begin()),
                     std::move_iterator(cid_key.end()));
    return Buffer{std::move(key_bytes)};
  }
}  // namespace fc::storage::piece
