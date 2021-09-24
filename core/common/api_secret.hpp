/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/file.hpp"
#include "primitives/jwt/jwt.hpp"

#include <jwt-cpp/jwt.h>
#include <libp2p/crypto/random_generator/boost_generator.hpp>

namespace fc {
  using libp2p::crypto::random::BoostRandomGenerator;
  using ApiAlgorithm = jwt::algorithm::hmacsha;
  using primitives::jwt::kPermissionKey;
  using primitives::jwt::kTokenType;
  using primitives::jwt::Permission;

  inline outcome::result<std::shared_ptr<ApiAlgorithm>> loadApiSecret(
      const boost::filesystem::path &path) {
    if (boost::filesystem::exists(path)) {
      OUTCOME_TRY(secret, common::readFile(path));
      return std::make_shared<jwt::algorithm::hs256>(
          std::string(common::span::cast<char>(secret.data()), secret.size()));
    }

    constexpr uint8_t kSecretSize = 32;
    std::string secret;
    secret.reserve(kSecretSize);
    BoostRandomGenerator{}.fillRandomly(secret, kSecretSize);
    OUTCOME_TRY(common::writeFile(path, common::span::cbytes(secret)));
    return std::make_shared<jwt::algorithm::hs256>(secret);
  }

  inline outcome::result<std::string> generateAuthToken(
      const std::shared_ptr<ApiAlgorithm> &algo,
      const std::vector<Permission> &perms) {
    std::error_code ec;
    auto token =
        jwt::create()
            .set_type(static_cast<std::string>(kTokenType))
            .set_payload_claim(static_cast<std::string>(kPermissionKey),
                               jwt::claim(perms.begin(), perms.end()))
            .sign(*algo, ec);

    if (ec) {
      return ec;
    }

    return token;
  }
}  // namespace fc
