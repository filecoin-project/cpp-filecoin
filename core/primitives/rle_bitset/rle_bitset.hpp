/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <set>

#include "codec/cbor/streams_annotation.hpp"
#include "codec/rle/rle_plus.hpp"
#include "common/outcome.hpp"

namespace fc::primitives {
  struct RleBitset : public std::set<uint64_t> {
    using set::set;

    inline RleBitset(set &&s) : set{s} {}

    inline bool has(uint64_t v) const {
      return find(v) != end();
    }

    inline void operator+=(const RleBitset &other) {
      insert(other.begin(), other.end());
    }

    inline void operator+=(const std::vector<RleBitset> &others) {
      for (const auto &other : others) {
        insert(other.begin(), other.end());
      }
    }

    inline RleBitset operator+(const RleBitset &other) const {
      auto result{*this};
      result += other;
      return result;
    }

    inline RleBitset operator+(const std::vector<RleBitset> &others) const {
      auto result{*this};
      for (const auto &other : others) {
        result += other;
      }
      return result;
    }

    inline void operator-=(const RleBitset &other) {
      for (const auto i : other) {
        erase(i);
      }
    }

    inline RleBitset operator-(const RleBitset &other) const {
      auto result{*this};
      result -= other;
      return result;
    }

    inline RleBitset cut(const RleBitset &to_cut) const {
      RleBitset result;
      uint64_t shift = 0;
      auto it = to_cut.begin();
      for (const auto element : *this) {
        while ((it != to_cut.end()) && (*it < element)) {
          ++shift;
          ++it;
        }
        if (it == to_cut.end() || *it > element) {
          result.insert(element - shift);
        }
      }
      return result;
    }

    inline RleBitset intersect(const RleBitset &other) const {
      RleBitset result;
      for (const auto i : other) {
        if (this->has(i)) {
          result.insert(i);
        }
      }
      return result;
    }

    inline RleBitset slice(uint64_t start, uint64_t count) const {
      std::vector<uint64_t> temp{begin(), end()};
      temp = {temp.begin() + start, temp.begin() + count};
      return RleBitset(temp.begin(), temp.end());
    }

    inline bool contains(const RleBitset &other) const {
      bool result = true;
      for (const auto i : other) {
        if (!this->has(i)) {
          result = false;
          break;
        }
      }
      return result;
    }

    inline bool containsAny(const RleBitset &other) const {
      bool result = false;
      for (const auto i : other) {
        if (this->has(i)) {
          result = true;
          break;
        }
      }
      return result;
    }
  };

  CBOR_ENCODE(RleBitset, set) {
    return s << codec::rle::encode(set);
  }

  CBOR_DECODE(RleBitset, set) {
    std::vector<uint8_t> rle;
    s >> rle;
    OUTCOME_EXCEPT(decoded, codec::rle::decode<RleBitset::value_type>(rle));
    set = RleBitset{std::move(decoded)};
    return s;
  }
}  // namespace fc::primitives
