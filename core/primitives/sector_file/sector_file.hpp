/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SECTOR_FILE_HPP
#define CPP_FILECOIN_SECTOR_FILE_HPP

#include <primitives/sector/sector.hpp>
#include <string>
#include <unordered_map>

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

  // 10x overheads
  const std::unordered_map<SectorFileType, int> kOverheadSeal{
      {SectorFileType::FTUnsealed, 10},
      {SectorFileType::FTSealed, 10},
      {SectorFileType::FTCache,
       70}  // TODO(artyom-yurin): [FIL-199] confirm for 32G
  };

  std::string toString(const SectorFileType &file_type);
  /**
   * Get amount of used memory for sealing
   * @param file_type - type of sector file
   * @param seal_proof_type - type of seal proof
   * @return amount of used memory
   */
  outcome::result<uint64_t> sealSpaceUse(SectorFileType file_type,
                                         RegisteredProof seal_proof_type);
  std::string sectorName(const SectorId &sid);

  struct SectorPaths {
   public:
    SectorId id;
    std::string unsealed;
    std::string sealed;
    std::string cache;

    void setPathByType(const SectorFileType &file_type,
                       const std::string &path);
    outcome::result<std::string> getPathByType(const SectorFileType &file_type);
  };

  enum class SectorFileTypeErrors {
    kInvalidSectorFileType = 1,
  };

}  // namespace fc::primitives::sector_file

OUTCOME_HPP_DECLARE_ERROR(fc::primitives::sector_file, SectorFileTypeErrors);

#endif  // CPP_FILECOIN_SECTOR_FILE_HPP
