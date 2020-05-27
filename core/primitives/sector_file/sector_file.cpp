/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector_file/sector_file.hpp"

namespace fc::primitives::sector_file {

  std::string toString(const SectorFileType &file_type) {
    switch (file_type) {
      case FTUnsealed:
        return "unsealed";
      case FTSealed:
        return "sealed";
      case FTCache:
        return "cache";
    }
    return "<unknown " + std::to_string(file_type) + ">";
  }

  outcome::result<uint64_t> sealSpaceUse(SectorFileType file_type,
                                         RegisteredProof seal_proof_type) {
    OUTCOME_TRY(sector_size,
                fc::primitives::sector::getSectorSize(seal_proof_type));

    uint64_t result = 0;
    for (const auto &type : kSectorFileTypes) {
      if ((file_type & type) == 0) {
        continue;
      }

      auto overhead_iter = kOverheadSeal.find(type);
      if (overhead_iter == kOverheadSeal.end()) {
        return SectorFileTypeErrors::InvalidSectorFileType;
      }

      result += overhead_iter->second * sector_size / 10;
    }
    return result;
  }

  void SectorPaths::setPathByType(const SectorFileType &file_type,
                                  const std::string &path) {
    switch (file_type) {
      case FTCache:
        cache = path;
        return;
      case FTUnsealed:
        unsealed = path;
        return;
      case FTSealed:
        sealed = path;
        return;
      default:
        return;
    }
  }

  std::string sectorName(const SectorId &sid) {
    return "s-t0" + std::to_string(sid.miner) + "-"
           + std::to_string(sid.sector);
  }

}  // namespace fc::primitives::sector_file

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives::sector_file,
                            SectorFileTypeErrors,
                            e) {
  using fc::primitives::sector_file::SectorFileTypeErrors;
  switch (e) {
    case (SectorFileTypeErrors::InvalidSectorFileType):
      return "SectorFileType: unsupported sector file type";
    default:
      return "SectorFileType: unknown error";
  }
}
