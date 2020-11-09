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

  outcome::result<SectorFileType> fromString(const std::string &file_type_str) {
    if (file_type_str == "unsealed") {
      return SectorFileType::FTUnsealed;
    }
    if (file_type_str == "sealed") {
      return SectorFileType::FTSealed;
    }
    if (file_type_str == "cache") {
      return SectorFileType::FTCache;
    }
    return SectorFileTypeErrors::kInvalidSectorFileType;
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
        return SectorFileTypeErrors::kInvalidSectorFileType;
      }

      result += overhead_iter->second * sector_size / kOverheadDenominator;
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

  outcome::result<std::string> SectorPaths::getPathByType(
      const SectorFileType &file_type) const {
    switch (file_type) {
      case FTCache:
        return cache;
      case FTUnsealed:
        return unsealed;
      case FTSealed:
        return sealed;
      default:
        return SectorFileTypeErrors::kInvalidSectorFileType;
    }
  }

  outcome::result<SectorId> parseSectorName(const std::string &sector_str) {
    SectorNumber sector_id;
    ActorId miner_id;

    auto count =
        std::sscanf(sector_str.c_str(), "s-t0%lld-%lld", &sector_id, &miner_id);

    if (count != 2) {
      return SectorFileTypeErrors::kInvalidSectorName;
    }

    return SectorId{
        .miner = miner_id,
        .sector = sector_id,
    };
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
    case (SectorFileTypeErrors::kInvalidSectorFileType):
      return "SectorFileType: unsupported sector file type";
    case (SectorFileTypeErrors::kInvalidSectorName):
      return "SectorFileType: cannot parse sector name";
    default:
      return "SectorFileType: unknown error";
  }
}
