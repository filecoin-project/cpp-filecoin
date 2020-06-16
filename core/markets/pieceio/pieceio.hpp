/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP
#define CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipld/walker.hpp"

namespace fc::markets::pieceio {

  using common::Buffer;
  using fc::storage::ipld::walker::Selector;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;

  enum class PiecePayloadLocation { IPLD_STORE, SEALED_SECTOR };

  class PieceIO {
   public:
    virtual ~PieceIO() = default;

    virtual outcome::result<std::pair<CID, UnpaddedPieceSize>>
    generatePieceCommitment(const RegisteredProof &registered_proof,
                            const CID &payload_cid,
                            const Selector &selector) = 0;

    virtual outcome::result<std::pair<CID, UnpaddedPieceSize>>
    generatePieceCommitment(const RegisteredProof &registered_proof,
                            const Buffer &piece) = 0;

    /**
     * @brief Locate payload block
     * @param payload_cid - id of the payload
     * @return { Storage type, CID of the parent piece }
     */
    virtual outcome::result<std::pair<PiecePayloadLocation, CID>>
    locatePiecePayload(const CID &payload_cid) const = 0;

    /**
     * @brief Obtain payload content
     * @param payload_cid - id of the payload
     * @return operation result
     */
    virtual outcome::result<Buffer> getPiecePayload(
        const CID &payload_cid) const = 0;
  };
}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP
