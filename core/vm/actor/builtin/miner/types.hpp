/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP

#include "adt/array.hpp"
#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "common/libp2p/peer/cbor_peer_id.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/miner/policy.hpp"

namespace fc::vm::actor::builtin::miner {
  using adt::UvarintKeyer;
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::OnChainPoStVerifyInfo;
  using primitives::sector::Proof;

  struct SectorPreCommitInfo {
    RegisteredProof registered_proof;
    SectorNumber sector;
    /// CommR
    CID sealed_cid;
    ChainEpoch seal_epoch;
    std::vector<DealId> deal_ids;
    /// Sector expiration
    ChainEpoch expiration;
  };
  CBOR_TUPLE(SectorPreCommitInfo,
             registered_proof,
             sector,
             sealed_cid,
             seal_epoch,
             deal_ids,
             expiration)

  struct SectorPreCommitOnChainInfo {
    SectorPreCommitInfo info;
    TokenAmount precommit_deposit;
    ChainEpoch precommit_epoch;
  };
  CBOR_TUPLE(SectorPreCommitOnChainInfo,
             info,
             precommit_deposit,
             precommit_epoch)

  struct SectorOnChainInfo {
    SectorPreCommitInfo info;
    /// Epoch at which SectorProveCommit is accepted
    ChainEpoch activation_epoch;
    /// Integral of active deals over sector lifetime, 0 if CommittedCapacity
    /// sector
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
  };
  CBOR_TUPLE(SectorOnChainInfo,
             info,
             activation_epoch,
             deal_weight,
             verified_deal_weight)

  struct WorkerKeyChange {
    /// Must be an ID address
    Address new_worker;
    ChainEpoch effective_at;
  };
  CBOR_TUPLE(WorkerKeyChange, new_worker, effective_at)

  struct MinerInfo {
    /**
     * Account that owns this miner.
     * - Income and returned collateral are paid to this address.
     * - This address is also allowed to change the worker address for the
     * miner.
     *
     * Must be an ID-address.
     */
    Address owner;
    /**
     * Worker account for this miner. The associated pubkey-type address is used
     * to sign blocks and messages on behalf of this miner. Must be an
     * ID-address.
     */
    Address worker;
    boost::optional<WorkerKeyChange> pending_worker_key;
    /// Libp2p identity that should be used when connecting to this miner.
    PeerId peer_id{codec::cbor::kDefaultT<PeerId>()};
    RegisteredProof seal_proof_type;
    /// Amount of space in each sector committed to the network by this miner.
    SectorSize sector_size;
    uint64_t window_post_partition_sectors;
  };
  CBOR_TUPLE(MinerInfo,
             owner,
             worker,
             pending_worker_key,
             peer_id,
             seal_proof_type,
             sector_size,
             window_post_partition_sectors)

  struct DeadlineInfo {
    ChainEpoch current_epoch;
    ChainEpoch period_start;
    uint64_t index;
    ChainEpoch open;
    ChainEpoch close;
    ChainEpoch challenge;
    ChainEpoch fault_cutoff;

    static DeadlineInfo make(ChainEpoch start, size_t id, ChainEpoch now);
    ChainEpoch nextPeriodStart() const {
      return period_start + kWPoStProvingPeriod;
    }
    bool elapsed() const {
      return current_epoch >= close;
    }
    bool faultCutoffPassed() const {
      return current_epoch >= fault_cutoff;
    }
    bool periodStarted() const {
      return current_epoch >= period_start;
    }
    ChainEpoch periodEnd() const {
      return period_start + kWPoStProvingPeriod - 1;
    }
  };

  struct Deadlines {
    std::vector<RleBitset> due;

    std::pair<size_t, size_t> count(size_t partition_size, size_t index) const {
      assert(index < due.size());
      auto sectors{due[index].size()};
      auto parts{sectors / partition_size};
      if (sectors % partition_size != 0) {
        ++parts;
      }
      return {parts, sectors};
    }

    std::pair<size_t, size_t> partitions(size_t part_size, size_t index) const {
      assert(index < due.size());
      size_t first_part{0};
      for (size_t i{0};; ++i) {
        auto [parts, sectors]{count(part_size, index)};
        if (i == index) {
          return {first_part, sectors};
        }
        first_part += parts;
      }
    }
  };
  CBOR_TUPLE(Deadlines, due)

  /// Balance of a Actor should equal exactly the sum of PreCommit deposits
  struct State {
    MinerInfo info;
    TokenAmount precommit_deposit;
    TokenAmount locked_funds;
    adt::Array<TokenAmount> vesting_funds;
    adt::Map<SectorPreCommitOnChainInfo, UvarintKeyer> precommitted_sectors;
    adt::Array<SectorOnChainInfo> sectors;
    ChainEpoch proving_period_start;
    RleBitset new_sectors;
    adt::Array<RleBitset> sector_expirations;
    CID deadlines;
    RleBitset fault_set;
    adt::Array<RleBitset> fault_epochs;
    RleBitset recoveries;
    RleBitset post_submissions;  // set of partition indices

    DeadlineInfo deadlineInfo(ChainEpoch now) const;
    outcome::result<void> addFaults(const RleBitset &sectors, ChainEpoch epoch);
    template <typename C>
    outcome::result<std::vector<SectorOnChainInfo>> getSectors(const C &c) {
      std::vector<SectorOnChainInfo> result;
      for (auto i : c) {
        OUTCOME_TRY(sector, sectors.get(i));
        result.push_back(std::move(sector));
      }
      return std::move(result);
    }
    template <typename Visitor>
    auto visitProvingSet(const Visitor &visitor) {
      return sectors.visit([&](auto id, auto &info) {
        if (fault_set.find(id) == fault_set.end()
            && recoveries.find(id) == recoveries.end()) {
          visitor(id, info);
        }
        return outcome::success();
      });
    }
    auto getDeadlines(IpldPtr ipld) {
      return ipld->getCbor<Deadlines>(deadlines);
    }
  };
  CBOR_TUPLE(State,
             info,
             precommit_deposit,
             locked_funds,
             vesting_funds,
             precommitted_sectors,
             sectors,
             proving_period_start,
             new_sectors,
             sector_expirations,
             deadlines,
             fault_set,
             fault_epochs,
             recoveries,
             post_submissions)
  using MinerActorState = State;

  enum class CronEventType {
    WorkerKeyChange,
    PreCommitExpiry,
    ProvingPeriod,
  };

  struct CronEventPayload {
    CronEventType event_type;
    boost::optional<RleBitset> sectors;
  };
  CBOR_TUPLE(CronEventPayload, event_type, sectors)
}  // namespace fc::vm::actor::builtin::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::miner::State> {
    template <typename Visitor>
    static void call(vm::actor::builtin::miner::State &state,
                     const Visitor &visit) {
      visit(state.vesting_funds);
      visit(state.precommitted_sectors);
      visit(state.sectors);
      visit(state.sector_expirations);
      visit(state.sector_expirations);
      visit(state.fault_epochs);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
