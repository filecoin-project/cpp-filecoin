/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

#include "vm/actor/builtin/types/type_manager/universal_impl.hpp"

#include "vm/actor/builtin/v0/miner/types/expiration.hpp"
#include "vm/actor/builtin/v2/miner/types/expiration.hpp"
#include "vm/actor/builtin/v3/miner/types/expiration.hpp"

#include "vm/actor/builtin/v4/todo.hpp"

#include "vm/actor/builtin/v0/miner/types/miner_info.hpp"
#include "vm/actor/builtin/v2/miner/types/miner_info.hpp"
#include "vm/actor/builtin/v3/miner/types/miner_info.hpp"

#include "vm/actor/builtin/types/miner/partition.hpp"
#include "vm/actor/builtin/v0/miner/types/partition.hpp"
#include "vm/actor/builtin/v2/miner/types/partition.hpp"
#include "vm/actor/builtin/v3/miner/types/partition.hpp"

namespace fc::vm::actor::builtin::types {
  using primitives::kChainEpochUndefined;

  outcome::result<Universal<ExpirationQueue>> TypeManager::loadExpirationQueue(
      const Runtime &runtime,
      const PartitionExpirationsArray &expirations_epochs,
      const QuantSpec &quant) {
    Universal<ExpirationQueue> eq{runtime.getActorVersion()};
    cbor_blake::cbLoadT(runtime.getIpfsDatastore(), eq);
    eq->queue = expirations_epochs;
    eq->quant = quant;
    return eq;
  }

  outcome::result<Universal<MinerInfo>> TypeManager::makeMinerInfo(
      const Runtime &runtime,
      const Address &owner,
      const Address &worker,
      const std::vector<Address> &control,
      const Buffer &peer_id,
      const std::vector<Multiaddress> &multiaddrs,
      const RegisteredSealProof &seal_proof_type,
      const RegisteredPoStProof &window_post_proof_type) {
    OUTCOME_TRY(sector_size,
                primitives::sector::getSectorSize(seal_proof_type));
    OUTCOME_TRY(partition_sectors,
                primitives::sector::getSealProofWindowPoStPartitionSectors(
                    seal_proof_type));

    Universal<MinerInfo> miner_info{runtime.getActorVersion()};

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

}  // namespace fc::vm::actor::builtin::types

UNIVERSAL_IMPL(miner::MinerInfo,
               v0::miner::MinerInfo,
               v2::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo)

UNIVERSAL_IMPL(miner::Partition,
               v0::miner::Partition,
               v2::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition,
               v3::miner::Partition)

UNIVERSAL_IMPL(miner::ExpirationQueue,
               v0::miner::ExpirationQueue,
               v2::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue,
               v3::miner::ExpirationQueue)
