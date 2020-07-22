/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_STORAGE_HPP
#define CPP_FILECOIN_PIECE_STORAGE_HPP

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::storage::piece {
  using primitives::DealId;
  using primitives::sector::SectorNumber;

  /**
   * Information about a single deal for a given piece
   */
  struct DealInfo {
    DealId deal_id;
    SectorNumber sector_id;
    uint64_t offset;
    uint64_t length;

    bool operator==(const DealInfo &) const;
  };
  CBOR_TUPLE(DealInfo, deal_id, sector_id, offset, length)

  /**
   * Metadata about a piece a provider may be storing based on its PieceCID --
   * so that, given a pieceCID during retrieval, the miner can determine how to
   * unseal it if needed
   */
  struct PieceInfo {
    CID piece_cid;
    std::vector<DealInfo> deals;

    bool operator==(const PieceInfo &) const;
  };
  CBOR_TUPLE(PieceInfo, piece_cid, deals)

  /* Location of the payload block inside the Piece */
  struct PayloadLocation {
    uint64_t relative_offset;
    uint64_t block_size;

    bool operator==(const PayloadLocation &) const;
  };
  CBOR_TUPLE(PayloadLocation, relative_offset, block_size)

  /* Information about parent Piece and block location */
  struct PayloadBlockInfo {
    CID parent_piece;
    PayloadLocation block_location;

    bool operator==(const PayloadBlockInfo &) const;
  };
  CBOR_TUPLE(PayloadBlockInfo, parent_piece, block_location);

  /**
   * Information about where a given payload with CID will live inside a piece
   */
  struct PayloadInfo {
    CID cid;
    std::vector<PayloadBlockInfo> piece_block_locations;
  };
  CBOR_TUPLE(PayloadInfo, cid, piece_block_locations);

  /**
   * @brief Piece consists of the number of payload blocks, each of them has
   * it's own CID. Piece CID is a CID of the Merkle tree root, constructed from
   * the 32-bytes segments of the whole area payload blocks.
   * @class PieceStorage provides functionality to:
   * - store information about Pieces: which sector contains selected Piece,
   * it's size and offset from the sector begin;
   * - store information about payload structure of the Piece: relative offset
   * of the payload block from the Piece begin and CID of the Piece, which
   * contains selected payload block;
   */
  class PieceStorage {
   public:
    /**
     * @brief Destructor
     */
    virtual ~PieceStorage() = default;

    /**
     * @brief Append new deal info for piece
     * @param piece_cid - id of the Piece
     * @param deal_info - info about deal
     * @return operation result
     */
    virtual outcome::result<void> addDealForPiece(
        const CID &piece_cid, const DealInfo &deal_info) = 0;

    /**
     * @brief Get info about selected Piece
     * @param piece_cid - id of the Piece
     * @return operation result
     */
    virtual outcome::result<PieceInfo> getPieceInfo(
        const CID &piece_cid) const = 0;

    /**
     * Retrieve the CIDInfo associated with `pieceCID` from the CID info store
     * @param payload_cid - cid of the payload
     * @return operation result
     */
    virtual outcome::result<PayloadInfo> getPayloadInfo(
        const CID &payload_cid) const = 0;

    /**
     * @brief Add locations of the payload blocks in the Piece
     * @param parent_piece - id of the Piece, which contains given payload
     * blocks
     * @param locations - { Payload block CID => PayloadLocation }
     */
    virtual outcome::result<void> addPayloadLocations(
        const CID &parent_piece, std::map<CID, PayloadLocation> locations) = 0;

    virtual outcome::result<PieceInfo> getPieceInfoFromCid(
        const CID &payload_cid,
        const boost::optional<CID> &piece_cid) const = 0;

    virtual outcome::result<bool> hasPieceInfo(
        CID payload_cid, const boost::optional<CID> &piece_cid) const = 0;

    virtual outcome::result<uint64_t> getPieceSize(
        CID payload_cid, const boost::optional<CID> &piece_cid) const = 0;
  };

}  // namespace fc::storage::piece

#endif
