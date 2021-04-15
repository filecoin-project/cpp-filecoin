/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
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
  };
  CBOR_TUPLE(PowerPair, raw, qa)

  struct VestingFunds {
    struct Fund {
      ChainEpoch epoch{};
      TokenAmount amount{};
    };
    std::vector<Fund> funds;
  };
  CBOR_TUPLE(VestingFunds::Fund, epoch, amount)
  CBOR_TUPLE(VestingFunds, funds)

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

  struct ExpirationSet {
    RleBitset on_time_sectors, early_sectors;
    TokenAmount on_time_pledge{};
    PowerPair active_power;
    PowerPair faulty_power;
  };
  CBOR_TUPLE(ExpirationSet,
             on_time_sectors,
             early_sectors,
             on_time_pledge,
             active_power,
             faulty_power)

  struct Partition {
    RleBitset sectors, faults, recoveries, terminated;
    adt::Array<ExpirationSet> expirations_epochs;  // quanted
    adt::Array<RleBitset> early_terminated;
    PowerPair live_power;
    PowerPair faulty_power;
    PowerPair recovering_power;
  };
  CBOR_TUPLE(Partition,
             sectors,
             faults,
             recoveries,
             terminated,
             expirations_epochs,
             early_terminated,
             live_power,
             faulty_power,
             recovering_power)

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

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::types::miner::Partition> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::Partition &p,
                     const Visitor &visit) {
      visit(p.expirations_epochs);
      visit(p.early_terminated);
    }
  };
}  // namespace fc
