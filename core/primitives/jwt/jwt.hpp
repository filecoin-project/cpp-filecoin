/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jwt-cpp/jwt.h>

namespace fc::primitives::jwt {
  using Permission = std::string;
  using ApiAlgorithm = ::jwt::algorithm::hmacsha;
  constexpr std::string_view kTokenType = "JWT";
  constexpr std::string_view kPermissionKey = "Allow";

  static const Permission kAdminPermission = "admin";
  static const Permission kReadPermission = "read";
  static const Permission kWritePermission = "write";
  static const Permission kSignPermission = "sign";

  static const std::vector<Permission> kAllPermission = {
      kAdminPermission, kReadPermission, kWritePermission, kSignPermission};

  static const std::vector<Permission> kDefaultPermission = {kReadPermission};
}  // namespace fc::primitives::jwt
