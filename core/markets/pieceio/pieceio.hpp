/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::markets::pieceio {
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::RegisteredSealProof;

  class PieceIO {
   public:
    virtual ~PieceIO() = default;

    virtual outcome::result<std::pair<CID, UnpaddedPieceSize>>
    generatePieceCommitment(const RegisteredSealProof &registered_proof,
                            const boost::filesystem::path &piece_path) = 0;
  };

}  // namespace fc::markets::pieceio
