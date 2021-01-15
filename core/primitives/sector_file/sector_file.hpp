/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include "common/logger.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/piece/piece_data.hpp"
#include "primitives/sector/sector.hpp"

using fc::primitives::piece::PaddedByteIndex;
using fc::primitives::piece::PaddedPieceSize;
using fc::primitives::piece::PieceData;
using fc::primitives::piece::PieceInfo;
using fc::primitives::piece::UnpaddedByteIndex;
using fc::primitives::piece::UnpaddedPieceSize;
using fc::primitives::sector::RegisteredProof;
using fc::primitives::sector::SectorId;

namespace fc::primitives::sector_file {
  enum SectorFileType : int {
    FTNone = 0,
    FTUnsealed = 1,
    FTSealed = 2,
    FTCache = 4,
  };
  constexpr size_t kSectorFileTypeBits{3};

  const std::vector<SectorFileType> kSectorFileTypes = {
      SectorFileType::FTUnsealed,
      SectorFileType::FTSealed,
      SectorFileType::FTCache};

  constexpr uint64_t kOverheadDenominator = 10;

  // 10x overheads
  const std::unordered_map<SectorFileType, uint64_t> kOverheadSeal{
      {SectorFileType::FTUnsealed, kOverheadDenominator},
      {SectorFileType::FTSealed, kOverheadDenominator},
      {SectorFileType::FTCache, 141}};

  const std::unordered_map<SectorFileType, uint64_t> kOverheadFinalized{
      {SectorFileType::FTUnsealed, kOverheadDenominator},
      {SectorFileType::FTSealed, kOverheadDenominator},
      {SectorFileType::FTCache, 2}};

  std::string toString(const SectorFileType &file_type);
  outcome::result<SectorFileType> fromString(const std::string &file_type_str);

  /**
   * Get amount of used memory for sealing
   * @param file_type - type of sector file
   * @param seal_proof_type - type of seal proof
   * @return amount of used memory
   */
  outcome::result<uint64_t> sealSpaceUse(SectorFileType file_type,
                                         RegisteredProof seal_proof_type);
  std::string sectorName(const SectorId &sid);
  outcome::result<SectorId> parseSectorName(const std::string &sector_str);

  struct SectorPaths {
   public:
    SectorId id;
    std::string unsealed;
    std::string sealed;
    std::string cache;

    void setPathByType(const SectorFileType &file_type,
                       const std::string &path);
    outcome::result<std::string> getPathByType(
        const SectorFileType &file_type) const;
  };

  class SectorFile {
   public:
    static outcome::result<std::shared_ptr<SectorFile>> createFile(
        const std::string &path, PaddedPieceSize max_piece_size);

    static outcome::result<std::shared_ptr<SectorFile>> openFile(
        const std::string &path, PaddedPieceSize max_piece_size);

    /**
     * 2 modes
     * if maybe_seal_proof_type is not none then write and compute PieceInfo -
     * result will be PieceInfo or error otherwise just write and result will be
     * boost::none or error
     */
    outcome::result<boost::optional<PieceInfo>> write(
        const PieceData &data,
        PaddedByteIndex offset,
        PaddedPieceSize size,
        const boost::optional<RegisteredProof> &maybe_seal_proof_type =
            boost::none);

    outcome::result<bool> read(const PieceData &output,
                               PaddedByteIndex offset,
                               PaddedPieceSize size);

    outcome::result<void> free(PaddedByteIndex offset, PaddedPieceSize size);

    gsl::span<const uint64_t> allocated() const;

    outcome::result<bool> hasAllocated(UnpaddedByteIndex offset,
                                       UnpaddedPieceSize size) const;

   private:
    outcome::result<void> markAllocated(PaddedByteIndex offset,
                                        PaddedPieceSize size);

    struct PadWriter {
     public:
      explicit PadWriter(std::fstream &output);

      outcome::result<void> write(gsl::span<const uint8_t> bytes);

      std::fstream &output_;
      std::vector<uint8_t> stash_;
      std::vector<uint8_t> work_;
    };

    SectorFile(std::string path,
               PaddedPieceSize max_size,
               std::vector<uint64_t> runs);

    std::fstream file_;
    std::vector<uint64_t> runs_;
    PaddedPieceSize max_size_;
    std::string path_;
    common::Logger logger_;
  };

  enum class SectorFileError {
    kFileNotExist = 1,
    kCannotOpenFile,
    kInvalidFile,
    kPipeNotOpen,
    kNotReadEnough,
    kNotWriteEnough,
    kCannotWrite,
    kCannotMoveCursor,
    kCannotRead,
    kCannotCreateFile,
    kCannotResizeFile,
    kInvalidRuns,
    kOversizeTrailer,
    kInvalidSize,
  };

  enum class SectorFileTypeErrors {
    kInvalidSectorFileType = 1,
    kInvalidSectorName,
  };

}  // namespace fc::primitives::sector_file

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector_file, SectorFileTypeErrors);
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector_file, SectorFileError);
