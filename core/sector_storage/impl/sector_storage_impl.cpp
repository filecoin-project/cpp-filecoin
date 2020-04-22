/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/sector_storage_impl.hpp"
#include <fcntl.h>
#include "sector_storage/sector_storage_error.hpp"

namespace fc::sector_storage {

  using fc::primitives::piece::PaddedPieceSize;
  using fc::primitives::sector_file::SectorFileTypes;
  using proofs = fc::proofs::Proofs;
  using fc::primitives::sector::getSectorSize;

  SectorStorageImpl::SectorStorageImpl(const std::string &root_path,
                                       RegisteredProof post_proof,
                                       RegisteredProof seal_proof)
      : root_(root_path),
        seal_proof_type_(seal_proof),
        post_proof_type_(post_proof) {
    size_ = 0;
    auto s1 = getSectorSize(seal_proof);
    if (s1.has_value()) {
      size_ = s1.value();
    }
  }

  outcome::result<SectorPaths> SectorStorageImpl::acquireSector(
      SectorId id,
      const SectorFileType &existing,
      const SectorFileType &allocate,
      bool sealing) {
    auto cache_path =
        root_ / path(SectorFileType(SectorFileTypes::FTCache).string());
    if (!(boost::filesystem::exists(cache_path)
          || boost::filesystem::create_directory(cache_path))) {
      return SectorStorageError::CANNOT_CREATE_DIR;
    }
    auto sealed_path =
        root_ / path(SectorFileType(SectorFileTypes::FTSealed).string());
    if (!(boost::filesystem::exists(sealed_path)
          || boost::filesystem::create_directory(sealed_path))) {
      return SectorStorageError::CANNOT_CREATE_DIR;
    }
    auto unsealed_path =
        root_ / path(SectorFileType(SectorFileTypes::FTUnsealed).string());
    if (!(boost::filesystem::exists(unsealed_path)
          || boost::filesystem::create_directory(unsealed_path))) {
      return SectorStorageError::CANNOT_CREATE_DIR;
    }

    SectorPaths result{
        .id = id,
    };

    std::vector<SectorFileTypes> types = {SectorFileTypes::FTCache,
                                          SectorFileTypes::FTSealed,
                                          SectorFileTypes::FTUnsealed};
    for (const auto &type : types) {
      if (!(existing.has(type) || allocate.has(type))) {
        continue;
      }

      path dir(SectorFileType(type).string());
      path file(fc::primitives::sector_file::sectorName(id));
      path full_path = root_ / dir / file;

      result.setPathByType(type, full_path.c_str());
    }

    return result;
  }

  outcome::result<PreCommit1Output> SectorStorageImpl::sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(
        paths,
        acquireSector(sector,
                      SectorFileTypes::FTUnsealed,
                      SectorFileTypes::FTSealed | SectorFileTypes::FTCache,
                      true));

    if (!boost::filesystem::exists(paths.sealed)) {
      boost::filesystem::ofstream sealed_file(paths.sealed);
      if (!sealed_file.is_open()) {
        return SectorStorageError::UNABLE_ACCESS_SEALED_FILE;
      }
      sealed_file.close();
    }

    if (!boost::filesystem::create_directory(paths.cache)) {
      if (boost::filesystem::exists(paths.cache)) {
        if (!boost::filesystem::remove_all(paths.cache)) {
          return SectorStorageError::CANNOT_REMOVE_DIR;
        }
        if (!boost::filesystem::create_directory(paths.cache)) {
          return SectorStorageError::CANNOT_CREATE_DIR;
        }
      } else {
        return SectorStorageError::CANNOT_CREATE_DIR;
      }
    }

    UnpaddedPieceSize sum;
    for (const auto &piece : pieces) {
      sum += piece.size.unpadded();
    }

    if (sum != PaddedPieceSize(size_).unpadded()) {
      return SectorStorageError::DONOT_MATCH_SIZES;
    }

    return proofs::sealPreCommitPhase1(seal_proof_type_,
                                       paths.cache,
                                       paths.unsealed,
                                       paths.sealed,
                                       sector.sector,
                                       sector.miner,
                                       ticket,
                                       pieces);
  }

  outcome::result<SectorCids> SectorStorageImpl::sealPreCommit2(
      const SectorId &sector, const PreCommit1Output &pc1o) {
    OUTCOME_TRY(
        paths,
        acquireSector(sector,
                      SectorFileTypes::FTSealed | SectorFileTypes::FTCache,
                      0,
                      true));

    return proofs::sealPreCommitPhase2(pc1o, paths.cache, paths.sealed);
  }

  outcome::result<Commit1Output> SectorStorageImpl::sealCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids)

  {
    OUTCOME_TRY(
        paths,
        acquireSector(sector,
                      SectorFileTypes::FTSealed | SectorFileTypes::FTCache,
                      0,
                      true));

    return proofs::sealCommitPhase1(seal_proof_type_,
                                    cids.sealed_cid,
                                    cids.unsealed_cid,
                                    paths.cache,
                                    paths.sealed,
                                    sector.sector,
                                    sector.miner,
                                    ticket,
                                    seed,
                                    pieces);
  }

  outcome::result<Proof> SectorStorageImpl::sealCommit2(
      const SectorId &sector, const Commit1Output &c1o)

  {
    return proofs::sealCommitPhase2(c1o, sector.sector, sector.miner);
  }

  outcome::result<void> SectorStorageImpl::finalizeSector(
      const SectorId &sector) {
    OUTCOME_TRY(paths,
                acquireSector(sector, SectorFileTypes::FTCache, 0, false));

    return proofs::clearCache(paths.cache);
  }

  outcome::result<PieceInfo> SectorStorageImpl::addPiece(
      SectorId sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      UnpaddedPieceSize new_piece_size,
      const PieceData &piece_data) {
    // open Pipe or just use fd

    SectorPaths staged_path;
    if (piece_sizes.empty()) {
      OUTCOME_TRY(staged_p,
                  acquireSector(sector, 0, SectorFileTypes::FTUnsealed, true));

      staged_path = staged_p;
      if (!boost::filesystem::exists(staged_path.unsealed)) {
        boost::filesystem::ofstream staged_file(staged_path.unsealed);
        if (!staged_file.is_open()) {
          return SectorStorageError::CANNOT_CREATE_FILE;
        }
        staged_file.close();
      }

    } else {
      OUTCOME_TRY(staged_p,
                  acquireSector(sector, SectorFileTypes::FTUnsealed, 0, true));
      staged_path = staged_p;
    }

    OUTCOME_TRY(response,
                proofs::writeWithAlignment(seal_proof_type_,
                                           piece_data,
                                           new_piece_size,
                                           staged_path.unsealed,
                                           piece_sizes));

    return PieceInfo(new_piece_size.padded(), response.piece_cid);
  }
}  // namespace fc::sector_storage
