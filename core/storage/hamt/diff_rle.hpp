/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/rle_bitset/rle_bitset.hpp"
#include "storage/hamt/diff.hpp"

namespace fc::hamt_diff {
  using primitives::RleBitset;

  struct RleMapDiff {
    RleBitset remove_keys;
    RleBitset add_keys;
    RleBitset change_keys;
    std::map<uint64_t, Bytes> add;
    std::map<uint64_t, std::pair<Bytes, Bytes>> change;

    bool operator()(uint64_t key, BytesIn value1, BytesIn value2) {
      if (value2.empty()) {
        remove_keys.insert(key);
      } else if (value1.empty()) {
        add_keys.insert(key);
        add.emplace(key, copy(value2));
      } else {
        change_keys.insert(key);
        change.emplace(key, std::make_pair(copy(value1), copy(value2)));
      }
      return true;
    }

    void rle(Bytes &out) const {
      const auto put{[&](const RleBitset &keys) {
        const auto buf{codec::rle::encode(keys)};
        codec::cbor::writeBytes(out, buf.size());
        append(out, buf);
      }};
      put(remove_keys);
      put(add_keys);
      put(change_keys);
    }
  };
}  // namespace fc::hamt_diff
