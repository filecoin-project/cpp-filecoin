/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jwt-cpp/jwt.h>
#include <libp2p/peer/peer_info.hpp>

#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "common/span.hpp"

namespace fc::api {
  using common::Buffer;
  using libp2p::peer::PeerInfo;
  using ApiAlgorithm = jwt::algorithm::hmacsha;
  const std::string kTokenType = "JWT";
  const std::string kPermissionKey = "Allow";

  using Permission = std::string;

  const Permission kAdminPermission = "admin";
  const Permission kReadPermission = "read";
  const Permission kWritePermission = "write";
  const Permission kSignPermission = "sign";

  const std::vector<Permission> kAllPermission = {
      kAdminPermission, kReadPermission, kWritePermission, kSignPermission};

  const std::vector<Permission> kDefaultPermission = {kReadPermission};

  struct CommonApi {
    /**
     * Creates auth token to the remote connection
     * @return auth token
     */
    API_METHOD(AuthNew, Buffer, const std::vector<Permission> &)
    /**
     * Verify auth token
     * @return allow permissions
     */
    API_METHOD(AuthVerify, std::vector<Permission>, const std::string &)

    /**
     * Returns listen addresses.
     */
    API_METHOD(NetAddrsListen, PeerInfo)

    /**
     * Initiates the connection to the peer.
     */
    API_METHOD(NetConnect, void, const PeerInfo &)

    /**
     * Returns all peers connected to the this host.
     */
    API_METHOD(NetPeers, std::vector<PeerInfo>)

    API_METHOD(Version, VersionResult)
  };

  template <typename A, typename F>
  void visitCommon(A &&a, const F &f) {
    f(a.AuthNew);
    f(a.AuthVerify);
    f(a.NetAddrsListen);
    f(a.NetConnect);
    f(a.NetPeers);
    f(a.Version);
  }

  inline void makeCommonApi(
      const std::shared_ptr<CommonApi> &api,
      const std::shared_ptr<ApiAlgorithm> &secret_algorithm,
      const common::Logger &logger) {
    api->AuthNew = [=](auto perms) -> outcome::result<Buffer> {
      std::error_code ec;
      auto token =
          jwt::create()
              .set_type(kTokenType)
              .set_payload_claim(kPermissionKey,
                                 jwt::claim(perms.begin(), perms.end()))
              .sign(*secret_algorithm, ec);

      if (ec) {
        logger->error("Error when sign token: {}", ec.message());
        return ERROR_TEXT("API ERROR");
      }

      return Buffer{common::span::cbytes(token)};
    };
    api->AuthVerify =
        [=](auto token) -> outcome::result<std::vector<Permission>> {
      static auto decode = [logger](const std::string &token)
          -> outcome::result<jwt::decoded_jwt<jwt::picojson_traits>> {
        try {
          return jwt::decode(token);
        } catch (const std::exception &e) {
          logger->error("Error when decode token: {}", e.what());
          return ERROR_TEXT("API ERROR");
        }
      };
      static auto verifier = jwt::verify()
                                 .with_type(kTokenType)
                                 .allow_algorithm(*secret_algorithm);

      OUTCOME_TRY(decoded_jwt, decode(token));
      std::error_code ec;
      verifier.verify(decoded_jwt, ec);
      if (ec) {
        logger->error("Error when verify token: {}", ec.message());
        return ERROR_TEXT("API ERROR");
      }
      auto perms_json =
          decoded_jwt.get_payload_claim(kPermissionKey).as_array();
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
