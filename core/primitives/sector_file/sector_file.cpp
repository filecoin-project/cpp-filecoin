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
