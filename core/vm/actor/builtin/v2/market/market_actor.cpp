/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/market/market_actor.hpp"

namespace fc::vm::actor::builtin::v2::market {

  ACTOR_METHOD_IMPL(VerifyDealsForActivation) {
    OUTCOME_TRY(result,
                v0::market::VerifyDealsForActivation::verifyDealsForActivation(
                    runtime, params));
    const auto &[deal_weight, verified_weight, deal_space] = result;

    return Result{deal_weight, verified_weight, deal_space};
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
}  // namespace fc::vm::actor::builtin::v2::market
