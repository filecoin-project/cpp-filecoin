/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::api {
  /** Is a binary encoded semver */
  using ApiVersion = uint64_t;

  struct VersionResult {
    std::string version;
    ApiVersion api_version;
    /** Block delay in seconds */
    uint64_t block_delay;

    inline bool operator==(const VersionResult &other) const {
      return version == other.version && api_version == other.api_version
             && block_delay == other.block_delay;
    }
  };

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
