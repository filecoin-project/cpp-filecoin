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

  outcome::result<void> PieceStorageImpl::addPieceInfo(const CID &piece_cid,
                                                       PieceInfo piece_info) {
    OUTCOME_TRY(key, piece_cid.toString());
    OUTCOME_TRY(value, codec::cbor::encode(piece_info));
    Buffer storage_key =
        PieceStorageImpl::convertKey(kPiecePrefix, std::move(key));
    OUTCOME_TRY(storage_->put(storage_key, value));
    return outcome::success();
  }

  outcome::result<PieceInfo> PieceStorageImpl::getPieceInfo(
      const CID &piece_cid) const {
    OUTCOME_TRY(key, piece_cid.toString());
    Buffer storage_key =
        PieceStorageImpl::convertKey(kPiecePrefix, std::move(key));
    OUTCOME_TRY(value, storage_->get(storage_key));
    OUTCOME_TRY(piece_info, codec::cbor::decode<PieceInfo>(value));
    return std::move(piece_info);
  }

  outcome::result<CidInfo> PieceStorageImpl::getCidInfo(
      const CID &piece_cid) const {
    // TODO (a.chernyshov) implement
    return PieceStorageError::kPieceNotFound;
  }

  outcome::result<void> PieceStorageImpl::addPayloadLocations(
      const CID &parent_piece, std::map<CID, PayloadLocation> locations) {
    for (auto &&[cid, location] : locations) {
      PayloadBlockInfo payload_info{.parent_piece = parent_piece,
                                    .block_location = location};
      OUTCOME_TRY(key, cid.toString());
      OUTCOME_TRY(value, codec::cbor::encode(payload_info));
      Buffer storage_key =
          PieceStorageImpl::convertKey(kLocationPrefix, std::move(key));
      OUTCOME_TRY(storage_->put(storage_key, value));
    }
    return outcome::success();
  }

  outcome::result<PayloadBlockInfo> PieceStorageImpl::getPayloadLocation(
      const CID &payload_cid) const {
    OUTCOME_TRY(key, payload_cid.toString());
    Buffer storage_key =
        PieceStorageImpl::convertKey(kLocationPrefix, std::move(key));
    OUTCOME_TRY(value, storage_->get(storage_key));
    OUTCOME_TRY(payload_info, codec::cbor::decode<PayloadBlockInfo>(value));
    return std::move(payload_info);
  }

  outcome::result<PieceInfo> PieceStorageImpl::getPieceInfoFromCid(
      const CID &payload_cid, const boost::optional<CID> &piece_cid) const {
    OUTCOME_TRY(cid_info, getCidInfo(payload_cid));
    for (auto &&block_location : cid_info.piece_block_locations) {
      OUTCOME_TRY(piece_info, getPieceInfo(block_location.parent_piece));
      if (piece_cid || piece_info.piece_cid == piece_cid.get()) {
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

  PieceStorageImpl::Buffer PieceStorageImpl::convertKey(std::string prefix,
                                                        std::string key) {
    std::vector<uint8_t> key_bytes{std::move_iterator(prefix.begin()),
                                   std::move_iterator(prefix.end())};
    key_bytes.insert(key_bytes.end(),
                     std::move_iterator(key.begin()),
                     std::move_iterator(key.end()));
    return Buffer{std::move(key_bytes)};
  }
}  // namespace fc::storage::piece
