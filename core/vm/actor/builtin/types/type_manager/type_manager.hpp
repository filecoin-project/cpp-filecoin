/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"
#include "vm/actor/builtin/types/type_manager/universal.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types {
  using libp2p::multi::Multiaddress;
  using miner::ExpirationQueuePtr;
  using miner::ExpirationSet;
  using miner::MinerInfo;
  using miner::QuantSpec;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using runtime::Runtime;

  class TypeManager {
   public:
    static outcome::result<ExpirationQueuePtr> loadExpirationQueue(
        Runtime &runtime,
        const adt::Array<ExpirationSet> &expirations_epochs,
        const QuantSpec &quant);

    static outcome::result<Universal<MinerInfo>> makeMinerInfo(
        Runtime &runtime,
        const Address &owner,
        const Address &worker,
        const std::vector<Address> &control,
        const Buffer &peer_id,
        const std::vector<Multiaddress> &multiaddrs,
        const RegisteredSealProof &seal_proof_type,
        const RegisteredPoStProof &window_post_proof_type);

   private:
    template <typename T>
    static std::shared_ptr<T> createLoadedExpirationQueuePtr(
        IpldPtr ipld,
        const adt::Array<ExpirationSet> &expirations_epochs,
        const QuantSpec &quant) {
      auto eq = std::make_shared<T>();
      eq->queue = expirations_epochs;
      eq->quant = quant;
      cbor_blake::cbLoadT(ipld, *eq);
      return eq;
    }
  };
}  // namespace fc::vm::actor::builtin::types
