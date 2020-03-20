/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP

#include <libp2p/multi/uvarint.hpp>

#include "adt/array.hpp"
#include "adt/map.hpp"
#include "adt/uvarint_key.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::vm::actor::builtin::miner {
  using adt::UvarintKey;
  using common::Buffer;
  using libp2p::multi::UVarint;
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
  using primitives::sector::RegisteredProof;

  using PeerId = std::string;

  struct PoStState {
    /// Epoch that starts the current proving period
    ChainEpoch proving_period_start;
    /**
     * Number of surprised post challenges that have been failed since last
     * successful PoSt. Indicates that the claimed storage power may not
     * actually be proven. Recovery can proceed by submitting a correct response
     * to a subsequent PoSt challenge, up until the limit of number of
     * consecutive failures.
     */
    uint64_t num_consecutive_failures;
  };
  CBOR_TUPLE(PoStState, proving_period_start, num_consecutive_failures)

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
  CBOR_TUPLE(
      SectorPreCommitInfo, sector, sealed_cid, seal_epoch, deal_ids, expiration)

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
    /// Fixed pledge collateral requirement determined at activation
    TokenAmount pledge_requirement;
    /// -1 if not currently declared faulted.
    ChainEpoch declared_fault_epoch;
    /// -1 if not currently declared faulted.
    ChainEpoch declared_fault_duration;
  };
  CBOR_TUPLE(SectorOnChainInfo,
             info,
             activation_epoch,
             deal_weight,
             pledge_requirement,
             declared_fault_epoch,
             declared_fault_duration)

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
    PeerId peer_id;
    /// Amount of space in each sector committed to the network by this miner.
    SectorSize sector_size;
  };
  CBOR_TUPLE(MinerInfo, owner, worker, pending_worker_key, peer_id, sector_size)

  /// Balance of a Actor should equal exactly the sum of PreCommit deposits
  struct MinerActorState {
    adt::Map<SectorPreCommitOnChainInfo, UvarintKey> precommitted_sectors;
    adt::Array<SectorOnChainInfo> sectors;
    RleBitset fault_set;
    adt::Array<SectorOnChainInfo> proving_set;
    MinerInfo info;
    PoStState post_state;
  };
  CBOR_TUPLE(MinerActorState,
             precommitted_sectors,
             sectors,
             fault_set,
             proving_set,
             info,
             post_state)

  enum class CronEventType {
    WindowedPoStExpiration,
    WorkerKeyChange,
    PreCommitExpiry,
    SectorExpiry,
    TempFault,
  };

  struct CronEventPayload {
    CronEventType event_type;
    boost::optional<RleBitset> sectors;
    RegisteredProof registered_proof{};
  };
  CBOR_TUPLE(CronEventPayload, event_type, sectors, registered_proof)
}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
