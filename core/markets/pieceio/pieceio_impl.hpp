/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP

#include "common/outcome.hpp"
#include "markets/pieceio/pieceio.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::markets::pieceio {
  using Ipld = fc::storage::ipfs::IpfsDatastore;

  class PieceIOImpl : public PieceIO {
   public:
    explicit PieceIOImpl(std::shared_ptr<Ipld> ipld);

    outcome::result<std::pair<CID, UnpaddedPieceSize>> generatePieceCommitment(
        const RegisteredProof &registered_proof,
        const CID &payload_cid,
        const Selector &selector) override;

    outcome::result<std::pair<CID, UnpaddedPieceSize>> generatePieceCommitment(
        const RegisteredProof &registered_proof, const Buffer &piece) override;

   private:
    std::shared_ptr<Ipld> ipld_;
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP
