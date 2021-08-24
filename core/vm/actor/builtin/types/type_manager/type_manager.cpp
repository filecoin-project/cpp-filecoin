/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::types {
  using primitives::kChainEpochUndefined;
  using types::miner::kWPoStPeriodDeadlines;

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

  outcome::result<Universal<Deadline>> TypeManager::makeEmptyDeadline(
      const Runtime &runtime, const CID &empty_amt_cid) {
    const auto version = runtime.getActorVersion();
    const auto ipld = runtime.getIpfsDatastore();

    Universal<Deadline> deadline{version};
    cbor_blake::cbLoadT(ipld, deadline);

    if (version < ActorVersion::kVersion3) {
      deadline->partitions = {empty_amt_cid, ipld};
      deadline->expirations_epochs = {empty_amt_cid, ipld};
    } else {
      OUTCOME_TRY(empty_partitions_cid, deadline->partitions.amt.flush());
      deadline->partitions_snapshot = {empty_partitions_cid, ipld};

      OUTCOME_TRY(empty_post_submissions_cid,
                  deadline->optimistic_post_submissions.amt.flush());
      deadline->optimistic_post_submissions_snapshot = {
          empty_post_submissions_cid, ipld};
    }

    return deadline;
  }

  outcome::result<Deadlines> TypeManager::makeEmptyDeadlines(
      const Runtime &runtime, const CID &empty_amt_cid) {
    OUTCOME_TRY(deadline, makeEmptyDeadline(runtime, empty_amt_cid));
    OUTCOME_TRY(deadline_cid, setCbor(runtime.getIpfsDatastore(), deadline));
    adt::CbCidT<Universal<Deadline>> deadline_cid_t{deadline_cid};
    return Deadlines{std::vector(kWPoStPeriodDeadlines, deadline_cid_t)};
  }

}  // namespace fc::vm::actor::builtin::types
