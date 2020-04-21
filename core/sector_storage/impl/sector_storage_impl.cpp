/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/sector_storage_impl.hpp"
#include "sector_storage/sector_storage_error.hpp"

namespace fc::sector_storage {
  using fc::primitives::sector_file::SectorFileTypes;

  SectorStorageImpl::SectorStorageImpl(const std::string &root_path)
      : root_(root_path) {}

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

  outcome::result<PreCommit1Output> sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    return outcome::success();
  }

  outcome::result<SectorCids> SectorStorageImpl::sealPreCommit2(
      const SectorId &sector, const PreCommit1Output &pc10) {
    return outcome::success();
  }

  outcome::result<Commit1Output> sealCommit1(const SectorId &sector,
                                             const SealRandomness &ticket,
                                             const InteractiveRandomness &seed,
                                             gsl::span<const PieceInfo> pieces,
                                             const SectorCids &cids)

  {
    return outcome::success();
  }

  outcome::result<Proof> SectorStorageImpl::sealCommit2(
      const SectorId &sector, const Commit1Output &c1o)

  {
    return outcome::success();
  }

  outcome::result<void> SectorStorageImpl::finalizeSector(
      const SectorId &sector) {
    return outcome::success();
  }

  outcome::result<PieceInfo> SectorStorageImpl::addPiece(
      SectorId sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      UnpaddedPieceSize new_piece_size,
      const PieceData &piece_data) {
    return outcome::success();
  }
}  // namespace fc::sector_storage
