/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RLE_BITSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RLE_BITSET_HPP

#include <set>

#include "codec/rle/rle_plus.hpp"
#include "common/outcome_throw.hpp"

namespace fc::primitives {
  struct RleBitset : public std::set<uint64_t> {
    using set::set;

    inline RleBitset(set &&s) : set{s} {}
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const RleBitset &set) {
    return s << codec::rle::encode(set);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, RleBitset &set) {
    std::vector<uint8_t> rle;
    s >> rle;
    auto decoded = codec::rle::decode<RleBitset::value_type>(rle);
    if (!decoded) {
      outcome::raise(decoded.error());
    }
    set = RleBitset{std::move(decoded.value())};
    return s;
  }
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RLE_BITSET_HPP
