/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/market/market_actor.hpp"

namespace fc::vm::actor::builtin::v3::market {

  ACTOR_METHOD_IMPL(VerifyDealsForActivation) {
    // todo
    return Result{{{0, 0, 0}}};
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<AddBalance>(),
      exportMethod<WithdrawBalance>(),
      exportMethod<PublishStorageDeals>(),
      exportMethod<VerifyDealsForActivation>(),
      exportMethod<ActivateDeals>(),
      exportMethod<OnMinerSectorsTerminate>(),
      exportMethod<ComputeDataCommitment>(),
      exportMethod<CronTick>(),
  };
}  // namespace fc::vm::actor::builtin::v3::market
