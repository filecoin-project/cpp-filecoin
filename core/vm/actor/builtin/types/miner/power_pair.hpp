/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::StoragePower;

  struct PowerPair {
    StoragePower raw{};
    StoragePower qa{};

    PowerPair() : raw(0), qa(0) {}
    PowerPair(StoragePower raw, StoragePower qa)
        : raw(std::move(raw)), qa(std::move(qa)) {}

    inline PowerPair operator+(const PowerPair &other) const {
      auto result{*this};
      result += other;
      return result;
    }

    inline PowerPair &operator+=(const PowerPair &other) {
      raw += other.raw;
      qa += other.qa;
      return *this;
    }

    inline PowerPair operator-(const PowerPair &other) const {
      auto result{*this};
      result -= other;
      return result;
    }

    inline PowerPair &operator-=(const PowerPair &other) {
      raw -= other.raw;
      qa -= other.qa;
      return *this;
    }

    inline bool operator==(const PowerPair &other) const {
      return (raw == other.raw) && (qa == other.qa);
    }

    inline bool operator!=(const PowerPair &other) const {
      return !(*this == other);
    }

    inline bool isZero() const {
      return (raw == 0) && (qa == 0);
    }

    inline PowerPair negative() const {
      auto result{*this};
      result.raw = -result.raw;
      result.qa = -result.qa;
      return result;
    }
  };
  CBOR_TUPLE(PowerPair, raw, qa)

}  // namespace fc::vm::actor::builtin::types::miner
