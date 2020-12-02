/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/rle/rle_plus_encoding_stream.hpp"

namespace fc::codec::rle {
  void RLEPlusEncodingStream::initContent() {
    content_.clear();
    content_.push_back(false);
    content_.push_back(false);
  }

  void RLEPlusEncodingStream::pushByte(uint8_t byte) {
    for (size_t i = 0; i < BYTE_BITS_COUNT; ++i) {
      bool bit = static_cast<bool>((byte & (1 << i)) >> i);
      content_.push_back(bit);
    }
  }

  Runs64 toRuns(const Set64 &set) {
    Runs64 runs;
    auto it{set.begin()};
    if (it != set.end()) {
      auto last{*it};
      ++it;
      runs.push_back(last);
      runs.push_back(1);
      while (it != set.end()) {
        auto current{*it};
        ++it;
        auto diff{current - last};
        if (diff == 1) {
          ++runs.back();
        } else {
          runs.push_back(diff - 1);
          runs.push_back(1);
        }
        last = current;
      }
    }
    return runs;
  }

  Set64 fromRuns(const Runs64 &runs) {
    Set64 set;
    uint64_t value{};
    bool include{false};
    for (auto run : runs) {
      if (include) {
        for (auto i{0u}; i < run; ++i) {
          set.insert(value + i);
        }
      }
      value += run;
      include = !include;
    }
    return set;
  }
}  // namespace fc::codec::rle
