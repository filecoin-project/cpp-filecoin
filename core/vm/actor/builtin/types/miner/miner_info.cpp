/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/miner_info.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::kChainEpochUndefined;
  using primitives::sector::getSealProofWindowPoStPartitionSectors;
  using primitives::sector::getSectorSize;

  outcome::result<Universal<MinerInfo>> makeMinerInfo(
      ActorVersion version,
      const Address &owner,
      const Address &worker,
      const std::vector<Address> &control,
      const Bytes &peer_id,
      const std::vector<Multiaddress> &multiaddrs,
      const RegisteredSealProof &seal_proof_type,
      const RegisteredPoStProof &window_post_proof_type) {
    SectorSize sector_size = 0;
    size_t partition_sectors = 0;

    if (version < ActorVersion::kVersion3) {
      OUTCOME_TRYA(sector_size, getSectorSize(seal_proof_type));
      OUTCOME_TRYA(partition_sectors,
                   getSealProofWindowPoStPartitionSectors(seal_proof_type));
    } else {
      OUTCOME_TRYA(sector_size, getSectorSize(window_post_proof_type));
      OUTCOME_TRYA(partition_sectors,
                   getWindowPoStPartitionSectors(window_post_proof_type));
    }

    Universal<MinerInfo> miner_info{version};

    miner_info->owner = owner;
    miner_info->worker = worker;
    miner_info->control = control;
    miner_info->pending_worker_key = boost::none;
    miner_info->peer_id = peer_id;
    miner_info->multiaddrs = multiaddrs;
    miner_info->seal_proof_type = seal_proof_type;
    miner_info->window_post_proof_type = window_post_proof_type;
    miner_info->sector_size = sector_size;
    miner_info->window_post_partition_sectors = partition_sectors;
    miner_info->consensus_fault_elapsed = kChainEpochUndefined;
    miner_info->pending_owner_address = boost::none;

    return miner_info;
  }

}  // namespace fc::vm::actor::builtin::types::miner
