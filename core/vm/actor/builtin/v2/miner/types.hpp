/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/v0/miner/types.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using common::Buffer;
  using fc::primitives::kChainEpochUndefined;
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using primitives::address::Address;
  using primitives::sector::Proof;
  using primitives::sector::RegisteredSealProof;
  using v0::miner::WorkerKeyChange;

  struct MinerInfo {
    static outcome::result<MinerInfo> make(
        const Address &owner,
        const Address &worker,
        const std::vector<Address> &control,
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddrs,
        const RegisteredSealProof &seal_proof_type) {
      OUTCOME_TRY(sector_size,
                  primitives::sector::getSectorSize(seal_proof_type));
      OUTCOME_TRY(partition_sectors,
                  primitives::sector::getSealProofWindowPoStPartitionSectors(
                      seal_proof_type));
      return MinerInfo{
          .owner = owner,
          .worker = worker,
          .control = control,
          .pending_worker_key = boost::none,
          .peer_id = peer_id,
          .multiaddrs = multiaddrs,
          .seal_proof_type = seal_proof_type,
          .sector_size = sector_size,
          .window_post_partition_sectors = partition_sectors,
          .consensus_fault_elapsed = kChainEpochUndefined,
          .pending_owner_address = boost::none,
      };
    }

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

    /**
     * Additional addresses that are permitted to submit messages controlling
     * this actor (optional). Must all be ID addresses.
     */
    std::vector<Address> control;

    boost::optional<WorkerKeyChange> pending_worker_key;

    /** Libp2p identity that should be used when connecting to this miner. */
    Buffer peer_id;

    /**
     * Slice of byte arrays representing Libp2p multi-addresses used for
     * establishing a connection with this miner.
     */
    std::vector<Multiaddress> multiaddrs;

    /** The proof type used by this miner for sealing sectors. */
    RegisteredSealProof seal_proof_type;

    /**
     * Amount of space in each sector committed to the network by this miner.
     * This is computed from the proof type and represented here redundantly.
     */
    SectorSize sector_size;

    /**
     * The number of sectors in each Window PoSt partition (proof). This is
     * computed from the proof type and represented here redundantly.
     */
    uint64_t window_post_partition_sectors;

    /**
     * The next epoch this miner is eligible for certain permissioned actor
     * methods and winning block elections as a result of being reported for a
     * consensus fault.
     */
    ChainEpoch consensus_fault_elapsed;

    /**
     * A proposed new owner account for this miner. Must be confirmed by a
     * message from the pending address itself.
     */
    boost::optional<Address> pending_owner_address;
  };
  CBOR_TUPLE(MinerInfo,
             owner,
             worker,
             control,
             pending_worker_key,
             peer_id,
             multiaddrs,
             seal_proof_type,
             sector_size,
             window_post_partition_sectors,
             consensus_fault_elapsed,
             pending_owner_address)

}  // namespace fc::vm::actor::builtin::v2::miner
