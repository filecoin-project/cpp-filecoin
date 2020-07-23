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
#include "storage/ipld/selector.hpp"

namespace fc::markets::pieceio {

  using common::Buffer;
  using fc::storage::ipld::Selector;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;

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
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP
