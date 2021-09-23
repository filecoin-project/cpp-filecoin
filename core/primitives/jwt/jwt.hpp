/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::primitives::jwt {
  using Permission = std::string;
  constexpr std::string_view kTokenType = "JWT";
  constexpr std::string_view kPermissionKey = "Allow";

  static const Permission kAdminPermission = "admin";
  static const Permission kReadPermission = "read";
  static const Permission kWritePermission = "write";
  static const Permission kSignPermission = "sign";

  static const std::vector<Permission> kAllPermission = {
      kAdminPermission, kReadPermission, kWritePermission, kSignPermission};

  static const std::vector<Permission> kDefaultPermission = {kReadPermission};

  bool hasPermission(const std::vector<Permission> &perms,
                     const Permission &need_permission) {
    return std::find(perms.begin(), perms.end(), need_permission)
           != perms.end();
  }
}  // namespace fc::primitives::jwt
