/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/common_api.hpp"
#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/api_secret.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "common/span.hpp"

namespace fc::api {
  using primitives::jwt::ApiAlgorithm;
  using primitives::jwt::kPermissionKey;
  using primitives::jwt::kTokenType;

  void fillAuthApi(const std::shared_ptr<CommonApi> &api,
                   const std::shared_ptr<ApiAlgorithm> &secret_algorithm,
                   const common::Logger &logger) {
    auto verifier = jwt::verify()
                        .with_type(static_cast<std::string>(kTokenType))
                        .allow_algorithm(*secret_algorithm);
    api->AuthNew = [secret_algorithm](auto perms) -> outcome::result<Buffer> {
      OUTCOME_TRY(token, generateAuthToken(secret_algorithm, perms));

      return Buffer{common::span::cbytes(token)};
    };
    api->AuthVerify =
        [verifier{std::move(verifier)},
         logger](auto token) -> outcome::result<std::vector<Permission>> {
      static auto decode = [logger](const std::string &token)
          -> outcome::result<jwt::decoded_jwt<jwt::picojson_traits>> {
        try {
          return jwt::decode(token);
        } catch (const std::exception &e) {
          logger->error("AuthVerify jwt decode: {}", e.what());
          return ERROR_TEXT("API ERROR");
        }
      };

      OUTCOME_TRY(decoded_jwt, decode(token));
      std::error_code ec;
      verifier.verify(decoded_jwt, ec);
      if (ec) {
        logger->error("AuthVerify jwt verify {:#}", ec);
        return ec;
      }
      auto perms_json =
          decoded_jwt
              .get_payload_claim(static_cast<std::string>(kPermissionKey))
              .as_array();
      std::vector<std::string> perms;
      std::transform(perms_json.begin(),
                     perms_json.end(),
                     std::back_inserter(perms),
                     [](const picojson::value &elem) -> std::string {
                       return elem.get<std::string>();
                     });

      return std::move(perms);
    };
  }
}  // namespace fc::api
