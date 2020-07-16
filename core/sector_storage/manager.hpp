/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP

#include "sector_storage/spec_interfaces/prover.hpp"
#include "sector_storage/spec_interfaces/sealer.hpp"
#include "sector_storage/spec_interfaces/storage.hpp"

using fc::primitives::SectorSize;
using fc::primitives::piece::PieceData;
using fc::primitives::piece::UnpaddedByteIndex;

namespace fc::sector_storage {
  class Manager : public Sealer, Storage, Prover {
   public:
    virtual SectorSize getSectorSize() = 0;

    virtual outcome::result<void> ReadPiece(const proofs::PieceData &output,
                                            const SectorId &sector,
                                            UnpaddedByteIndex offset,
                                            const UnpaddedPieceSize &size,
                                            const SealRandomness &randomness,
                                            const CID &cid) = 0;
  };
}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_MANAGER_HPP
