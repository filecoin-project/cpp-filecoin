/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::api {
  using ApiVersion = uint64_t;

  /**
   * Make API semantic version
   * @param major - major version
   * @param minor - minor version
   * @param patch - patch version
   * @return api version
   */
  inline constexpr ApiVersion makeApiVersion(uint32_t major,
                                             uint32_t minor,
                                             uint32_t patch) {
    return major << 16 | minor << 8 | patch;
  }
}  // namespace fc::api
