/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/setup_common.hpp"
#include "common/file.hpp"

#include <random>

namespace fc {
  using primitives::jwt::ApiAlgorithm;
  using primitives::jwt::kAllPermission;
  using primitives::jwt::kPermissionKey;
  using primitives::jwt::kTokenType;

  outcome::result<std::shared_ptr<ApiAlgorithm>> loadApiSecret(
      const boost::filesystem::path &path) {
    if (boost::filesystem::exists(path)) {
      OUTCOME_TRY(secret, common::readFile(path));
      return std::make_shared<jwt::algorithm::hs256>(
          std::string(common::span::cast<char>(secret.data()), secret.size()));
    }

    constexpr uint8_t kSecretSize = 32;
    std::string secret;
    secret.reserve(kSecretSize);
    std::generate_n(
        std::back_insert_iterator(secret),
        kSecretSize,
        std::independent_bits_engine<std::random_device, 8, uint8_t>{});
    OUTCOME_TRY(common::writeFile(path, common::span::cbytes(secret)));
    return std::make_shared<jwt::algorithm::hs256>(secret);
  }

  outcome::result<std::string> generateAuthToken(
      const std::shared_ptr<ApiAlgorithm> &algo) {
    std::error_code ec;
    auto token =
        jwt::create()
            .set_type(static_cast<std::string>(kTokenType))
            .set_payload_claim(
                static_cast<std::string>(kPermissionKey),
                jwt::claim(kAllPermission.begin(), kAllPermission.end()))
            .sign(*algo, ec);

    if (ec) {
      return ec;
    }

    return token;
  }
}  // namespace fc
