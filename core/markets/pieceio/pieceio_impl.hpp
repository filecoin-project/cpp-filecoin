/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP

#include "markets/pieceio/pieceio.hpp"

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::markets::pieceio {

  class PieceIOImpl : public PieceIO {
   public:
    explicit PieceIOImpl(const boost::filesystem::path &temp_dir);

    outcome::result<std::pair<CID, UnpaddedPieceSize>> generatePieceCommitment(
        const RegisteredSealProof &registered_proof,
        const boost::filesystem::path &path) override;

   private:
    boost::filesystem::path temp_dir_;
  };

}  // namespace fc::markets::pieceio

#endif  // CPP_FILECOIN_CORE_MARKETS_PIECEIO_PIECEIO_IMPL_HPP
