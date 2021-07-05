/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/uvarint.hpp>

namespace fc {
  using libp2p::multi::UVarint;

  struct CidPrefix {
    uint64_t version{};
    uint64_t codec{};
    uint64_t mh_type{};
    int mh_length{};

    inline std::vector<uint8_t> toBytes() const {
      std::vector<uint8_t> prefix;
      const auto version_encoded = UVarint(version).toVector();
      prefix.insert(
          prefix.end(), version_encoded.begin(), version_encoded.end());
      const auto codec_encoded = UVarint(codec).toVector();
      prefix.insert(prefix.end(), codec_encoded.begin(), codec_encoded.end());
      const auto type_encoded = UVarint(mh_type).toVector();
      prefix.insert(prefix.end(), type_encoded.begin(), type_encoded.end());
      const auto size_encoded = UVarint(mh_length).toVector();
      prefix.insert(prefix.end(), size_encoded.begin(), size_encoded.end());
      return prefix;
    }

    bool operator==(const CidPrefix &other) const {
      return version == other.version && codec == other.codec
             && mh_type == other.mh_type && mh_length == other.mh_length;
    }
  };
  FC_OPERATOR_NOT_EQUAL(CidPrefix)
}  // namespace fc
