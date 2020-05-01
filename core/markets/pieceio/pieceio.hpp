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
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/walker.hpp"

namespace fc::markets::pieceio {

  using fc::storage::ipld::walker::Selector;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredProof;
  using Ipld = fc::storage::ipfs::IpfsDatastore;

  class PieceIO {
   public:
    explicit PieceIO(std::shared_ptr<Ipld> ipld);

    virtual ~PieceIO() = default;

    outcome::result<std::pair<CID, UnpaddedPieceSize>> generatePieceCommitment(
        const RegisteredProof &registered_proof,
        const CID &payload_cid,
        const Selector &selector);

   private:
    std::shared_ptr<Ipld> ipld_;
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_HPP
