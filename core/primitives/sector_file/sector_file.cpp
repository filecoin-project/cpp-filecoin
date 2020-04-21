/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/sector_file/sector_file.hpp"

namespace fc::primitives::sector_file {

  SectorFileType::SectorFileType(int type) : data_(type) {}

  SectorFileType &SectorFileType::operator=(int rhs) {
    data_ = rhs;
    return *this;
  }

  SectorFileType::operator int() const {
    return data_;
  }

  std::string SectorFileType::string() const {
    switch (data_) {
      case FTUnsealed:
        return "unsealed";
      case FTSealed:
        return "sealed";
      case FTCache:
        return "cache";
    }
    return "<unknown " + std::to_string(data_) + ">";
  }

  bool SectorFileType::has(const SectorFileType &single_type) const {
    return (data_ & single_type) == single_type;
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
      return "s-t0" + std::to_string(sid.miner) + "-" + std::to_string(sid.sector);
  }

}  // namespace fc::primitives::sector_file
