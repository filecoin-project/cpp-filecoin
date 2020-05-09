/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_STORAGE_IMPL_HPP
#define CPP_FILECOIN_PIECE_STORAGE_IMPL_HPP

#include <memory>
#include <string>

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "storage/face/persistent_map.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::storage::piece {
  const std::string kPiecePrefix = "/storagemarket/pieces/";
  const std::string kLocationPrefix = "/storagemarket/cid-infos/";

  class PieceStorageImpl : public PieceStorage {
   protected:
    using Buffer = common::Buffer;
    using PersistentMap = storage::face::PersistentMap<Buffer, Buffer>;

   public:
    PieceStorageImpl(std::shared_ptr<PersistentMap> storage_backend)
        : storage_{std::move(storage_backend)} {}

    outcome::result<void> addPieceInfo(const CID &piece_cid,
                                       PieceInfo piece_info) override;

    outcome::result<PieceInfo> getPieceInfo(
        const CID &piece_cid) const override;

    outcome::result<void> addPayloadLocations(
        const CID &parent_piece,
        std::map<CID, PayloadLocation> locations) override;

    outcome::result<PayloadBlockInfo> getPayloadLocation(
        const CID &paload_cid) const override;

   private:
    std::shared_ptr<PersistentMap> storage_;

    /**
     * @brief Convert string key to byte buffer
     * @param prefix - key namespace
     * @param key - data to convert
     * @return byte buffer
     */
    static Buffer convertKey(std::string prefix, std::string key);
  };

  CBOR_TUPLE(PieceInfo, deal_id, sector_id, offset, length)

  CBOR_TUPLE(PayloadLocation, relative_offset, block_size)

  CBOR_TUPLE(PayloadBlockInfo, parent_piece, block_location)
}  // namespace fc::storage::piece

#endif
