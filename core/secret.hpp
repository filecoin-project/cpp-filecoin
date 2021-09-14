/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "sector_storage/stores/storage.hpp"
#include "sector_storage/stores/storage_error.hpp"

#include <fstream>

namespace fc {
  using api::ApiAlgorithm;
  using api::kAllPermission;
  using api::kPermissionKey;
  using api::kTokenType;
  using sector_storage::stores::LocalStorage;
  using sector_storage::stores::StorageError;

  outcome::result<std::shared_ptr<ApiAlgorithm>> getApiSecret(
      const std::shared_ptr<LocalStorage> &storage,
      const common::Logger &logger) {
    auto maybe_secret = storage->getSecret();
    if (maybe_secret.has_value()) {
      return std::make_shared<jwt::algorithm::hs256>(maybe_secret.value());
    } else if (maybe_secret == outcome::failure(StorageError::kFileNotExist)) {
      std::string secret(32, '0');
      std::fstream rnd_file("/dev/random");
      rnd_file.read(secret.data(), secret.size());
      OUTCOME_TRY(storage->setSecret(secret));

      auto algo = std::make_shared<jwt::algorithm::hs256>(secret);

      std::error_code ec;
      auto token = jwt::create()
                       .set_type(kTokenType)
                       .set_payload_claim(kPermissionKey,
                                          jwt::claim(kAllPermission.begin(),
                                                     kAllPermission.end()))
                       .sign(*algo, ec);

      if (ec) {
        logger->error("Error when sign token: {}", ec.message());
        return ERROR_TEXT("ERROR");
      }

      OUTCOME_TRY(storage->setApiToken(token));

      return algo;
    } else {
      return maybe_secret.error();
    }
  }

}  // namespace fc
