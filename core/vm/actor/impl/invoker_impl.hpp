/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP

#include "vm/actor/invoker.hpp"

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  /// Finds and loads actor code, invokes actor methods
  class InvokerImpl : public Invoker {
   public:
    InvokerImpl();

    ~InvokerImpl() override = default;

    void config(const StoragePower &min_verified_deal_size,
                const StoragePower &consensus_miner_min_power,
                const std::vector<RegisteredProof> &supported_proofs) override;

    outcome::result<InvocationOutput> invoke(const Actor &actor,
                                             Runtime &runtime) override;

   private:
    std::map<CID, ActorExports> builtin_;
  };
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INVOKER_IMPL_HPP
