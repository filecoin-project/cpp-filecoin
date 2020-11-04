/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor {
  using primitives::StoragePower;
  using primitives::sector::RegisteredProof;
  using runtime::InvocationOutput;

  /// Finds and loads actor code, invokes actor methods
  class Invoker {
   public:
    virtual ~Invoker() = default;

    /**
     * Configure actors
     * @param min_verified_deal_size - miner actor minimal verified deal size
     * @param consensus_miner_min_power - miner actor consensus minimal power
     * @param supported_proofs - proofs that are supported by power actor
     */
    virtual void config(
        const StoragePower &min_verified_deal_size,
        const StoragePower &consensus_miner_min_power,
        const std::vector<RegisteredProof> &supported_proofs) = 0;

    virtual outcome::result<InvocationOutput> invoke(
        const Actor &actor, const std::shared_ptr<Runtime> &runtime) = 0;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_HPP
