/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem.hpp>
#include "common/logger.hpp"
#include "sector_storage/stores/store_error.hpp"

namespace fc::sector_storage::stores {
  namespace fs = boost::filesystem;

  inline outcome::result<std::string> tempFetchDest(
      const std::string &dest,
      bool allow_creation,
      const common::Logger &logger) {
    static std::string kFetchTempName = "fetching";
    fs::path dest_path(dest);
    auto temp_dir = dest_path.parent_path() / kFetchTempName;
    if (allow_creation) {
      boost::system::error_code ec;
      fs::create_directories(temp_dir, ec);
      if (ec.failed()) {
        logger->error(ec.message());
        return StoreError::kCannotCreateDir;
      }
    }
    return (temp_dir / dest_path.filename()).string();
  }
}  // namespace fc::sector_storage::stores
