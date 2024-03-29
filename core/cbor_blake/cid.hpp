/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/bytes.hpp"
#include "crypto/blake2/blake2b160.hpp"

namespace fc {
  constexpr BytesN<6> kCborBlakePrefix{0x01, 0x71, 0xA0, 0xE4, 0x02, 0x20};

  using common::Hash256;

  struct CbCid : Hash256 {
    CbCid() = default;
    explicit CbCid(const Hash256 &hash) : Hash256{hash} {}
    static CbCid hash(BytesIn cbor) {
      return CbCid{crypto::blake2b::blake2b_256(cbor)};
    }
  };
  using CbCidsIn = gsl::span<const CbCid>;
}  // namespace fc

template <>
struct std::hash<fc::CbCid> {
  auto operator()(const fc::CbCid &cid) const {
    return std::hash<fc::Hash256>()(cid);
  }
};
