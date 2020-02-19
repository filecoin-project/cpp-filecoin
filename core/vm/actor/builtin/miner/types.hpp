/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP

#include <libp2p/multi/uvarint.hpp>

#include "primitives/address/address_codec.hpp"
#include "primitives/big_int.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "proofs/proofs.hpp"

namespace fc::vm::actor::builtin::miner {
  using libp2p::multi::UVarint;
  using primitives::BigInt;
  using primitives::RleBitset;
  using primitives::address::Address;
  using proofs::Comm;

  struct PoStState {
    uint64_t proving_period_start;
    uint64_t num_consecutive_failures;
  };

  struct SectorPreCommitInfo {
    uint64_t sector;
    CID sealed_cid;
    uint64_t seal_epoch;
    std::vector<uint64_t> deal_ids;
    uint64_t expiration;
  };

  struct SectorPreCommitOnChainInfo {
    SectorPreCommitInfo info;
    BigInt precommit_deposit;
    uint64_t precommit_epoch;
  };

  struct SectorOnChainInfo {
    SectorPreCommitInfo info;
    uint64_t activation_epoch;
    BigInt deal_weight;
    BigInt pledge_requirement;
    uint64_t declared_fault_epoch;
    uint64_t declared_fault_duration;
  };

  struct WorkerKeyChange {
    Address new_worker;
    uint64_t effective_at;
  };

  struct MinerInfo {
    Address owner;
    Address worker;
    boost::optional<WorkerKeyChange> pending_worker_key;
    std::string peer_id;
    uint64_t sector_size;
  };

  struct MinerActorState {
    CID precommitted_sectors;
    CID sectors;
    RleBitset fault_set;
    CID proving_set;
    MinerInfo info;
    PoStState post_state;
  };

  CBOR_ENCODE(PoStState, state) {
    return s << (s.list() << state.proving_period_start
                          << state.num_consecutive_failures);
  }

  CBOR_DECODE(PoStState, state) {
    s.list() >> state.proving_period_start >> state.num_consecutive_failures;
    return s;
  }

  CBOR_ENCODE(SectorPreCommitInfo, info) {
    return s << (s.list() << info.sector << info.sealed_cid << info.seal_epoch
                          << info.deal_ids << info.expiration);
  }

  CBOR_DECODE(SectorPreCommitInfo, info) {
    s.list() >> info.sector >> info.sealed_cid >> info.seal_epoch
        >> info.deal_ids >> info.expiration;
    return s;
  }

  CBOR_ENCODE(SectorPreCommitOnChainInfo, info) {
    return s << (s.list() << info.info << info.precommit_deposit
                          << info.precommit_epoch);
  }

  CBOR_DECODE(SectorPreCommitOnChainInfo, info) {
    s.list() >> info.info >> info.precommit_deposit >> info.precommit_epoch;
    return s;
  }

  CBOR_ENCODE(SectorOnChainInfo, info) {
    return s << (s.list() << info.info << info.activation_epoch
                          << info.deal_weight << info.pledge_requirement
                          << info.declared_fault_epoch
                          << info.declared_fault_duration);
  }

  CBOR_DECODE(SectorOnChainInfo, info) {
    s.list() >> info.info >> info.activation_epoch >> info.deal_weight
        >> info.pledge_requirement >> info.declared_fault_epoch
        >> info.declared_fault_duration;
    return s;
  }

  CBOR_ENCODE(WorkerKeyChange, change) {
    return s << (s.list() << change.new_worker << change.effective_at);
  }

  CBOR_DECODE(WorkerKeyChange, change) {
    s.list() >> change.new_worker >> change.effective_at;
    return s;
  }

  CBOR_ENCODE(MinerInfo, info) {
    return s << (s.list() << info.owner << info.worker
                          << info.pending_worker_key << info.peer_id
                          << info.sector_size);
  }

  CBOR_DECODE(MinerInfo, info) {
    s.list() >> info.owner >> info.worker >> info.pending_worker_key
        >> info.peer_id >> info.sector_size;
    return s;
  }

  CBOR_ENCODE(MinerActorState, state) {
    return s << (s.list() << state.precommitted_sectors << state.sectors
                          << state.fault_set << state.proving_set << state.info
                          << state.post_state);
  }

  CBOR_DECODE(MinerActorState, state) {
    s.list() >> state.precommitted_sectors >> state.sectors >> state.fault_set
        >> state.proving_set >> state.info >> state.post_state;
    return s;
  }
}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_TYPES_HPP
