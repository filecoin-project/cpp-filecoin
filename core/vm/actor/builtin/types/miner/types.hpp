/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using primitives::sector::RegisteredSealProof;

  /**
   * Type used in actor method parameters
   */
  struct SectorDeclaration {
    /**
     * The deadline to which the sectors are assigned, in range
     * [0..WPoStPeriodDeadlines)
     */
    uint64_t deadline{0};

    /** Partition index within the deadline containing the sectors. */
    uint64_t partition{0};

    /** Sectors in the partition being declared faulty. */
    RleBitset sectors;
  };
  CBOR_TUPLE(SectorDeclaration, deadline, partition, sectors)

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

  struct SectorPreCommitInfo {
    RegisteredSealProof registered_proof;
    SectorNumber sector{};
    /// CommR
    CID sealed_cid;
    ChainEpoch seal_epoch{};
    std::vector<DealId> deal_ids;
    /// Sector expiration
    ChainEpoch expiration{};
    bool replace_capacity = false;
    uint64_t replace_deadline{};
    uint64_t replace_partition{};
    SectorNumber replace_sector{};
  };
  CBOR_TUPLE(SectorPreCommitInfo,
             registered_proof,
             sector,
             sealed_cid,
             seal_epoch,
             deal_ids,
             expiration,
             replace_capacity,
             replace_deadline,
             replace_partition,
             replace_sector)

  struct SectorPreCommitOnChainInfo {
    SectorPreCommitInfo info;
    TokenAmount precommit_deposit{};
    ChainEpoch precommit_epoch{};
    DealWeight deal_weight{};
    DealWeight verified_deal_weight{};
  };
  CBOR_TUPLE(SectorPreCommitOnChainInfo,
             info,
             precommit_deposit,
             precommit_epoch,
             deal_weight,
             verified_deal_weight)

  struct SectorOnChainInfo {
    SectorNumber sector{};
    RegisteredSealProof seal_proof;
    CID sealed_cid;
    std::vector<DealId> deals;
    ChainEpoch activation_epoch{};
    ChainEpoch expiration{};
    DealWeight deal_weight{};
    DealWeight verified_deal_weight{};
    TokenAmount init_pledge{};
    TokenAmount expected_day_reward{};
    TokenAmount expected_storage_pledge{};
  };
  CBOR_TUPLE(SectorOnChainInfo,
             sector,
             seal_proof,
             sealed_cid,
             deals,
             activation_epoch,
             expiration,
             deal_weight,
             verified_deal_weight,
             init_pledge,
             expected_day_reward,
             expected_storage_pledge)

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
