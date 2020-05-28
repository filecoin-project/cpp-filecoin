/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/sector_storage_impl.hpp"
#include <fcntl.h>
#include "sector_storage/sector_storage_error.hpp"

namespace fc::sector_storage {

  using fc::primitives::piece::PaddedPieceSize;
  using fc::primitives::sector_file::SectorFileType;
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
      SectorId id, SectorFileType sector_type) {
    SectorPaths result{
        .id = id,
    };

    for (const auto &type : primitives::sector_file::kSectorFileTypes) {
      if ((sector_type & type) == 0) {
        continue;
      }

      auto dir_path = root_ / path(toString(type));
      if (!(boost::filesystem::exists(dir_path)
            || boost::filesystem::create_directory(dir_path))) {
        return SectorStorageError::CANNOT_CREATE_DIR;
      }

      path file(primitives::sector_file::sectorName(id));

      result.setPathByType(type, (dir_path / file).c_str());
    }

    return result;
  }

  outcome::result<PreCommit1Output> SectorStorageImpl::sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    OUTCOME_TRY(paths,
                acquireSector(
                    sector,
                    static_cast<SectorFileType>(SectorFileType::FTSealed
                                                | SectorFileType::FTCache
                                                | SectorFileType::FTUnsealed)));

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
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache)));

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
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache)));

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
    OUTCOME_TRY(paths, acquireSector(sector, SectorFileType::FTCache));

    return proofs::clearCache(size_, paths.cache);
  }

  outcome::result<PieceInfo> SectorStorageImpl::addPiece(
      SectorId sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      UnpaddedPieceSize new_piece_size,
      const PieceData &piece_data) {
    // open Pipe or just use fd

    OUTCOME_TRY(staged_path, acquireSector(sector, SectorFileType::FTUnsealed));

    if (piece_sizes.empty()) {
      if (!boost::filesystem::exists(staged_path.unsealed)) {
        boost::filesystem::ofstream staged_file(staged_path.unsealed);
        if (!staged_file.is_open()) {
          return SectorStorageError::CANNOT_CREATE_FILE;
        }
        staged_file.close();
      }
    }

    OUTCOME_TRY(response,
                proofs::writeWithAlignment(seal_proof_type_,
                                           piece_data,
                                           new_piece_size,
                                           staged_path.unsealed,
                                           piece_sizes));

    return PieceInfo(new_piece_size.padded(), response.piece_cid);
  }

  outcome::result<PieceData> SectorStorageImpl::readPieceFromSealedSector(
      const SectorId &sector,
      UnpaddedByteIndex offset,
      UnpaddedPieceSize size,
      const SealRandomness &ticket,
      const CID &unsealedCID) {
    OUTCOME_TRY(path, acquireSector(sector, SectorFileType::FTUnsealed));
    if (!boost::filesystem::exists(path.unsealed)) {
      boost::filesystem::ofstream unsealed_file(path.unsealed);
      if (!unsealed_file.is_open()) {
        return SectorStorageError::CANNOT_CREATE_FILE;
      }
      unsealed_file.close();

      OUTCOME_TRY(sealed,
                  acquireSector(
                      sector,
                      static_cast<SectorFileType>(SectorFileType::FTSealed
                                                  | SectorFileType::FTCache)));

      OUTCOME_TRY(proofs::unseal(seal_proof_type_,
                                 sealed.cache,
                                 sealed.sealed,
                                 path.unsealed,
                                 sector.sector,
                                 sector.miner,
                                 ticket,
                                 unsealedCID));
    }

    if (offset + size > boost::filesystem::file_size(path.unsealed)) {
      return SectorStorageError::OUT_OF_FILE_SIZE;
    }

    if (size == boost::filesystem::file_size(path.unsealed)) {
      return PieceData(path.unsealed);
    }

    int piece[2];
    if (pipe(piece) < 0) return SectorStorageError::CANNOT_CREATE_FILE;

    constexpr uint64_t chunk_size = 256;

    std::ifstream unsealed_file(path.unsealed, std::ifstream::binary);

    if (!unsealed_file.is_open()) {
      close(piece[0]);
      close(piece[1]);
      return SectorStorageError::CANNOT_OPEN_FILE;
    }

    unsealed_file.seekg(offset, unsealed_file.beg);

    char *buffer = new char[chunk_size];

    for (uint64_t read_size = 0; read_size < size;) {
      uint64_t curr_read_size = std::min(chunk_size, size - read_size);

      // read data as a block:
      unsealed_file.read(buffer, curr_read_size);

      write(piece[1], buffer, unsealed_file.gcount());

      read_size += unsealed_file.gcount();
    }

    delete[] buffer;

    close(piece[1]);

    unsealed_file.close();

    return PieceData(piece[0]);
  }
}  // namespace fc::sector_storage
