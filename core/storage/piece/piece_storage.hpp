/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PIECE_STORAGE_HPP
#define CPP_FILECOIN_PIECE_STORAGE_HPP

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::storage::piece {
  /* Information about Piece location and it's deal */
  struct PieceInfo {
    uint64_t deal_id;
    uint64_t sector_id;
    uint64_t offset;
    uint64_t length;

    bool operator==(const PieceInfo&) const;
  };

  /* Location of the payload block inside the Piece */
  struct PayloadLocation {
    uint64_t relative_offset;
    uint64_t block_size;

    bool operator==(const PayloadLocation&) const;
  };

  /* Information about parent Piece and block location */
  struct PayloadBlockInfo {
    CID parent_piece;
    PayloadLocation block_location;

    bool operator==(const PayloadBlockInfo&) const;
  };

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
     * @brief Add new piece info
     * @param piece_cid - id of the Piece
     * @param piece_info - info about Piece
     * @return operation result
     */
    virtual outcome::result<void> addPieceInfo(const CID &piece_cid,
                                               PieceInfo piece_info) = 0;

    /**
     * @brief Get info about selected Piece
     * @param piece_cid - id of the Piece
     * @return operation result
     */
    virtual outcome::result<PieceInfo> getPieceInfo(
        const CID &piece_cid) const = 0;

    /**
     * @brief Add locations of the payload blocks in the Piece
     * @param parent_piece - id of the Piece, which contains given payload
     * blocks
     * @param locations - { Payload block CID => PayloadLocation }
     */
    virtual outcome::result<void> addPayloadLocations(
        const CID &parent_piece, std::map<CID, PayloadLocation> locations) = 0;

    /**
     * @brief Get location of the payload block
     * @param paload_cid - id of the payload block
     * @return operation result
     */
    virtual outcome::result<PayloadBlockInfo> getPayloadLocation(
        const CID &paload_cid) const = 0;
  };

  /**
   * @enum Piece storage errors
   */
  enum class PieceStorageError {
    STORAGE_BACKEND_ERROR,
    PIECE_NOT_FOUND,
    PAYLOAD_NOT_FOUND
  };
}  // namespace fc::storage::piece

OUTCOME_HPP_DECLARE_ERROR(fc::storage::piece, PieceStorageError);

#endif
