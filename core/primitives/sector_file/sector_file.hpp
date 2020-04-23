/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SECTOR_FILE_HPP
#define CPP_FILECOIN_SECTOR_FILE_HPP

#include <primitives/sector/sector.hpp>
#include <string>

using fc::primitives::sector::SectorId;

namespace fc::primitives::sector_file {
  enum SectorFileType : int {
    FTUnsealed = 1,
    FTSealed = 2,
    FTCache = 4,
  };

  std::string toString(const SectorFileType &file_type);
  std::string sectorName(const SectorId &sid);

  struct SectorPaths {
   public:
    SectorId id;
    std::string unsealed;
    std::string sealed;
    std::string cache;

    void setPathByType(const SectorFileType &file_type,
                       const std::string &path);
  };
}  // namespace fc::primitives::sector_file

#endif  // CPP_FILECOIN_SECTOR_FILE_HPP
