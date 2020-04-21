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

  SectorStorageImpl::SectorStorageImpl(const std::string &root_path,
                                       RegisteredProof post_proof,
                                       RegisteredProof seal_proof,
                                       SectorSize sector_size)
      : root_(root_path),
        seal_proof_type_(seal_proof),
        post_proof_type_(post_proof),
        size_(sector_size) {}

  outcome::result<SectorPaths> SectorStorageImpl::acquireSector(
      SectorId id,
      const SectorFileType &existing,
      const SectorFileType &allocate,
      bool sealing) {
    boost::system::error_code ec;
    auto cache_path =
        root_ / path(SectorFileType(SectorFileTypes::FTCache).string());
    if (!(boost::filesystem::exists(cache_path, ec)
          || boost::filesystem::create_directory(cache_path, ec))) {
      return SectorStorageError::CANNOT_CREATE_DIR;
    }
    auto sealed_path =
        root_ / path(SectorFileType(SectorFileTypes::FTSealed).string());
    if (!(boost::filesystem::exists(sealed_path, ec)
          || boost::filesystem::create_directory(sealed_path, ec))) {
      return SectorStorageError::CANNOT_CREATE_DIR;
    }
    auto unsealed_path =
        root_ / path(SectorFileType(SectorFileTypes::FTUnsealed).string());
    if (!(boost::filesystem::exists(unsealed_path, ec)
          || boost::filesystem::create_directory(unsealed_path, ec))) {
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

      // TODO: Allocate file

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

    int fd;
    if ((fd = open(paths.sealed.c_str(), O_RDWR | O_CREAT)) == -1) {
      return SectorStorageError::UNABLE_ACCESS_SEALED_FILE;
    }
    if (close(fd)) {
      return SectorStorageError::CANNOT_CLOSE_FILE;
    }

    boost::system::error_code ec;
    if (!boost::filesystem::create_directory(paths.cache, ec)) {
      if (boost::filesystem::exists(paths.cache, ec)) {
        if (!boost::filesystem::remove_all(paths.cache, ec)) {
          return SectorStorageError::CANNOT_REMOVE_DIR;
        }
        if (!boost::filesystem::create_directory(paths.cache, ec)) {
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
    return outcome::success();
  }
}  // namespace fc::sector_storage
