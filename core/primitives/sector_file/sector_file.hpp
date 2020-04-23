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

  class SectorFileType {
   public:
    SectorFileType(int type);

    operator int() const;

    SectorFileType &operator=(int rhs);

    std::string string() const;

    bool has(const SectorFileType &single_type) const;

   private:
    int data_;
  };

  enum SectorFileTypes {
    FTUnsealed = 1,
    FTSealed = 2,
    FTCache = 4,
  };

  struct SectorPaths {
   public:
    SectorId id;
    std::string unsealed;
    std::string sealed;
    std::string cache;

    void setPathByType(const SectorFileType &file_type,
                       const std::string &path);
  };

  std::string sectorName(const SectorId &sid);
}  // namespace fc::primitives::sector_file

#endif  // CPP_FILECOIN_SECTOR_FILE_HPP
