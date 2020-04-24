/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_HPP

#include <boost/filesystem.hpp>
#include "sector_storage/sector_storage.hpp"

using boost::filesystem::path;
using fc::primitives::SectorSize;
using fc::proofs::RegisteredProof;

namespace fc::sector_storage {

  class SectorStorageImpl : public SectorStorage {
   public:
    SectorStorageImpl(const std::string &root_path,
                      RegisteredProof post_proof,
                      RegisteredProof seal_proof);

    outcome::result<SectorPaths> acquireSector(SectorId id,
                                               SectorFileType sector_type) override;

    outcome::result<PreCommit1Output> sealPreCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        gsl::span<const PieceInfo> pieces) override;

    outcome::result<SectorCids> sealPreCommit2(
        const SectorId &sector, const PreCommit1Output &pc1o) override;

    outcome::result<Commit1Output> sealCommit1(
        const SectorId &sector,
        const SealRandomness &ticket,
        const InteractiveRandomness &seed,
        gsl::span<const PieceInfo> pieces,
        const SectorCids &cids) override;

    outcome::result<Proof> sealCommit2(const SectorId &sector,
                                       const Commit1Output &c1o) override;

    outcome::result<void> finalizeSector(const SectorId &sector) override;

    outcome::result<PieceInfo> addPiece(
        SectorId sector,
        gsl::span<const UnpaddedPieceSize> piece_sizes,
        UnpaddedPieceSize new_piece_size,
        const PieceData &piece_data) override;

   private:
    path root_;
    RegisteredProof seal_proof_type_;
    RegisteredProof post_proof_type_;
    SectorSize size_;
  };
}  // namespace fc::sector_storage
#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_IMPL_HPP
