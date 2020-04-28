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
#include "storage/ipld/ipld_node.hpp"

namespace fc::markets::pieceio {

  using fc::storage::ipld::IPLDNode;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;

  class PieceIO {
   public:
    virtual ~PieceIO() = default;

    outcome::result<std::pair<CID, UnpaddedPieceSize>> generatePieceCommitment(
        const RegisteredProof &registered_proof,
        const CID &payload_cid,
        std::shared_ptr<IPLDNode> &selector);

    outcome::result<CID> readPiece();
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP
