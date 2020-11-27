/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/cgo/cgo_invoker.hpp"
#include "vm/actor/cgo/actors.hpp"

namespace fc::vm::actor::cgo {

  CgoInvoker::CgoInvoker() {}

  void CgoInvoker::config(
      const StoragePower &min_verified_deal_size,
      const StoragePower &consensus_miner_min_power,
      const std::vector<RegisteredProof> &supported_proofs) {
    ::fc::vm::actor::cgo::config(
        min_verified_deal_size, consensus_miner_min_power, supported_proofs);
  }

  outcome::result<InvocationOutput> CgoInvoker::invoke(
      const Actor &actor, const std::shared_ptr<Runtime> &runtime) {
    return ::fc::vm::actor::cgo::invoke(actor.code, runtime);
  }

}  // namespace fc::vm::actor::cgo
