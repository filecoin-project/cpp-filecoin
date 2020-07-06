/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_STORAGE_HPP
#define CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_STORAGE_HPP

#include "primitives/piece/piece_data.hpp"

namespace fc::sector_storage {
  class Storage {
   public:
    virtual ~Storage() = default;

    // Add a piece to an existing *unsealed* sector
    virtual outcome::result<PieceInfo> addPiece(
        const SectorId &sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        const UnpaddedPieceSize &new_piece_size,
        const proofs::PieceData &piece_data) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_SECTOR_STROAGE_SPEC_INTERFACES_STORAGE_HPP
