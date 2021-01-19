/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_CGO_CGO_INVOKER_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_CGO_CGO_INVOKER_HPP

#include "vm/actor/invoker.hpp"

namespace fc::vm::actor::cgo {

  /**
   * Invoker implementation that invokes GoLang specs-actors using cgo
   */
  class CgoInvoker : public Invoker {
   public:
    explicit CgoInvoker();

    void config(
        const StoragePower &min_verified_deal_size,
        const StoragePower &consensus_miner_min_power,
        const std::vector<RegisteredSealProof> &supported_proofs) override;

    outcome::result<InvocationOutput> invoke(
        const Actor &actor, const std::shared_ptr<Runtime> &runtime) override;
  };

}  // namespace fc::vm::actor::cgo

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_CGO_CGO_INVOKER_HPP
