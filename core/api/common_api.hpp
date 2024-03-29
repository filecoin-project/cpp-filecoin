/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "api/utils.hpp"
#include "api/version.hpp"
#include "common/bytes.hpp"
#include "primitives/jwt/jwt.hpp"

namespace fc::api {
  using libp2p::peer::PeerInfo;
  using primitives::jwt::Permission;
  namespace jwt = primitives::jwt;

  struct CommonApi {
    /**
     * Creates auth token to the remote connection
     * @return auth token
     */
    API_METHOD(AuthNew,
               jwt::kAdminPermission,
               Bytes,
               const std::vector<Permission> &)
    /**
     * Verify auth token
     * @return allow permissions
     */
    API_METHOD(AuthVerify,
               jwt::kReadPermission,
               std::vector<Permission>,
               const std::string &)

    API_METHOD(Version, jwt::kReadPermission, VersionResult)

    API_METHOD(Session, jwt::kReadPermission, std::string)
  };

  template <typename A, typename F>
  void visitCommon(A &&a, const F &f) {
    f(a.AuthNew);
    f(a.AuthVerify);
    f(a.Session);
    f(a.Version);
  }
}  // namespace fc::api
