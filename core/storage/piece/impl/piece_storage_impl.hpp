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
#include "storage/piece/impl/piece_storage_error.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::storage::piece {
  using common::Buffer;
  const std::string kPiecePrefix = "/storagemarket/pieces/";
  const std::string kLocationPrefix = "/storagemarket/cid-infos/";

  class PieceStorageImpl : public PieceStorage {
   protected:
    using PersistentMap = storage::face::PersistentMap<Buffer, Buffer>;

   public:
    PieceStorageImpl(std::shared_ptr<PersistentMap> storage_backend);

    outcome::result<void> addDealForPiece(const CID &piece_cid,
                                          const DealInfo &deal_info) override;

    outcome::result<PieceInfo> getPieceInfo(
        const CID &piece_cid) const override;

    outcome::result<void> addPayloadLocations(
        const CID &parent_piece,
        std::map<CID, PayloadLocation> locations) override;

    outcome::result<PayloadInfo> getPayloadInfo(
        const CID &piece_cid) const override;

    outcome::result<PieceInfo> getPieceInfoFromCid(
        const CID &payload_cid,
        const boost::optional<CID> &piece_cid) const override;

    outcome::result<bool> hasPieceInfo(
        CID payload_cid, const boost::optional<CID> &piece_cid) const override;

    outcome::result<uint64_t> getPieceSize(
        CID payload_cid, const boost::optional<CID> &piece_cid) const override;

   private:
    std::shared_ptr<PersistentMap> storage_;

    /**
     * @brief Make a byte buffer key from cid with string prefix
     * @param prefix - key namespace
     * @param cid - data to convert
     * @return byte buffer
     */
    static outcome::result<Buffer> makeKey(const std::string &prefix,
                                           const CID &cid);
  };

}  // namespace fc::storage::piece

#endif
