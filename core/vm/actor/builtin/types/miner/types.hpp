/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::PoStProof;

  struct PowerPair {
    StoragePower raw{};
    StoragePower qa{};

    PowerPair() : raw(0), qa(0) {}
    PowerPair(const StoragePower &raw, const StoragePower &qa)
        : raw(raw), qa(qa) {}

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
  FC_OPERATOR_NOT_EQUAL(PowerPair)
  CBOR_TUPLE(PowerPair, raw, qa)

  struct WorkerKeyChange {
    /// Must be an ID address
    Address new_worker;
    ChainEpoch effective_at{};

    inline bool operator==(const WorkerKeyChange &other) const {
      return new_worker == other.new_worker
             && effective_at == other.effective_at;
    }
  };
  CBOR_TUPLE(WorkerKeyChange, new_worker, effective_at)

  enum class CronEventType {
    kWorkerKeyChange,
    kProvingDeadline,
    kProcessEarlyTerminations,
  };

  struct CronEventPayload {
    CronEventType event_type;
  };
  CBOR_TUPLE(CronEventPayload, event_type)

  struct WindowedPoSt {
    /**
     * Partitions proved by this WindowedPoSt.
     */
    RleBitset partitions;

    /**
     * Array of proofs, one per distinct registered proof type present in the
     * sectors being proven. In the usual case of a single proof type, this
     * array will always have a single element (independent of number of
     * partitions).
     */
    std::vector<PoStProof> proofs;
  };
  CBOR_TUPLE(WindowedPoSt, partitions, proofs)
}  // namespace fc::vm::actor::builtin::types::miner
