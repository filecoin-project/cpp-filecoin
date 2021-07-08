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

#include "vm/actor/builtin/types/storage_power/claim.hpp"
#include "vm/actor/builtin/v0/storage_power/types/claim.hpp"
#include "vm/actor/builtin/v2/storage_power/types/claim.hpp"
#include "vm/actor/builtin/v3/storage_power/types/claim.hpp"

#include "vm/actor/builtin/v0/miner/types/miner_info.hpp"
#include "vm/actor/builtin/v2/miner/types/miner_info.hpp"
#include "vm/actor/builtin/v3/miner/types/miner_info.hpp"

namespace fc::vm::actor::builtin::types {
  using primitives::kChainEpochUndefined;

  outcome::result<ExpirationQueuePtr> TypeManager::loadExpirationQueue(
      Runtime &runtime,
      const adt::Array<ExpirationSet> &expirations_epochs,
      const QuantSpec &quant) {
    const auto version = runtime.getActorVersion();

    switch (version) {
      case ActorVersion::kVersion0:
        return createLoadedExpirationQueuePtr<v0::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion2:
        return createLoadedExpirationQueuePtr<v2::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion3:
        return createLoadedExpirationQueuePtr<v3::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  outcome::result<Universal<MinerInfo>> TypeManager::makeMinerInfo(
      Runtime &runtime,
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

UNIVERSAL_IMPL(storage_power::Claim,
               v0::storage_power::Claim,
               v2::storage_power::Claim,
               v3::storage_power::Claim,
               v3::storage_power::Claim,
               v3::storage_power::Claim)

UNIVERSAL_IMPL(miner::MinerInfo,
               v0::miner::MinerInfo,
               v2::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo,
               v3::miner::MinerInfo)
