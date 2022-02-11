/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace ledger::filecoin {

  struct VersionInfo {
    uint8_t appMode{};
    uint8_t major{};
    uint8_t minor{};
    uint8_t patch{};

    inline bool operator==(const VersionInfo &other) const {
      return appMode == other.appMode && major == other.major
             && minor == other.minor && patch == other.patch;
    }

    inline bool operator!=(const VersionInfo &other) const {
      return !(*this == other);
    }

    inline bool operator<(const VersionInfo &other) const {
      return std::tie(major, minor, patch)
             < std::tie(other.major, other.minor, other.patch);
    }

    inline bool operator>(const VersionInfo &other) const {
      return other < *this;
    }

    inline bool operator<=(const VersionInfo &other) const {
      return !(*this > other);
    }

    inline bool operator>=(const VersionInfo &other) const {
      return !(*this < other);
    }

    inline std::string ToString() const {
      return std::to_string(major) + "." + std::to_string(minor) + "."
             + std::to_string(patch);
    }
  };

}  // namespace ledger::filecoin
