/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/worker_key_change.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"
#include "vm/actor/version.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::SectorSize;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;

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

    /**
     * Additional addresses that are permitted to submit messages controlling
     * this actor (optional). Must all be ID addresses.
     */
    std::vector<Address> control;

    boost::optional<WorkerKeyChange> pending_worker_key;

    /** Libp2p identity that should be used when connecting to this miner. */
    Bytes peer_id;

    /**
     * Slice of byte arrays representing Libp2p multi-addresses used for
     * establishing a connection with this miner.
     */
    std::vector<Multiaddress> multiaddrs;

    /** The proof type used by this miner for sealing sectors. */
    RegisteredSealProof seal_proof_type{RegisteredSealProof::kUndefined};

    /**
     * The proof type used for Window PoSt for this miner.
     * A miner may commit sectors with different seal proof types (but
     * compatible sector size and corresponding PoSt proof types).
     */
    RegisteredPoStProof window_post_proof_type{RegisteredPoStProof::kUndefined};

    /**
     * Amount of space in each sector committed to the network by this miner.
     * This is computed from the proof type and represented here redundantly.
     */
    SectorSize sector_size{};

    /**
     * The number of sectors in each Window PoSt partition (proof). This is
     * computed from the proof type and represented here redundantly.
     */
    uint64_t window_post_partition_sectors{};

    /**
     * The next epoch this miner is eligible for certain permissioned actor
     * methods and winning block elections as a result of being reported for a
     * consensus fault.
     */
    ChainEpoch consensus_fault_elapsed{};

    /**
     * A proposed new owner account for this miner. Must be confirmed by a
     * message from the pending address itself.
     */
    boost::optional<Address> pending_owner_address;

    inline bool operator==(const MinerInfo &other) const {
      return owner == other.owner && worker == other.worker
             && control == other.control
             && pending_worker_key == other.pending_worker_key
             && peer_id == other.peer_id && multiaddrs == other.multiaddrs
             && seal_proof_type == other.seal_proof_type
             && window_post_proof_type == other.window_post_proof_type
             && sector_size == other.sector_size
             && window_post_partition_sectors
                    == other.window_post_partition_sectors
             && consensus_fault_elapsed == other.consensus_fault_elapsed
             && pending_owner_address == other.pending_owner_address;
    }

    inline bool operator!=(const MinerInfo &other) const {
      return !(*this == other);
    }
  };

  outcome::result<Universal<MinerInfo>> makeMinerInfo(
      ActorVersion version,
      const Address &owner,
      const Address &worker,
      const std::vector<Address> &control,
      const Bytes &peer_id,
      const std::vector<Multiaddress> &multiaddrs,
      const RegisteredSealProof &seal_proof_type,
      const RegisteredPoStProof &window_post_proof_type);

}  // namespace fc::vm::actor::builtin::types::miner
