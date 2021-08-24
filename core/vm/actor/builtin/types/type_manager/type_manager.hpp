/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/miner/deadlines.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types {
  using libp2p::multi::Multiaddress;
  using miner::Deadline;
  using miner::Deadlines;
  using miner::ExpirationQueue;
  using miner::MinerInfo;
  using miner::PartitionExpirationsArray;
  using miner::QuantSpec;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;

  class TypeManager {
   public:
    static outcome::result<Universal<ExpirationQueue>> loadExpirationQueue(
        const Runtime &runtime,
        const PartitionExpirationsArray &expirations_epochs,
        const QuantSpec &quant);

    static outcome::result<Universal<MinerInfo>> makeMinerInfo(
        const Runtime &runtime,
        const Address &owner,
        const Address &worker,
        const std::vector<Address> &control,
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddrs,
        const RegisteredSealProof &seal_proof_type,
        const RegisteredPoStProof &window_post_proof_type);

    static outcome::result<Universal<Deadline>> makeEmptyDeadline(
        const Runtime &runtime, const CID &empty_amt_cid);

    static outcome::result<Deadlines> makeEmptyDeadlines(
        const Runtime &runtime, const CID &empty_amt_cid);
  };

}  // namespace fc::vm::actor::builtin::types
