/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/market/market_actor.hpp"

#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using states::MarketActorStatePtr;
  using toolchain::Toolchain;

  ACTOR_METHOD_IMPL(VerifyDealsForActivation) {
    const auto address_matcher =
        Toolchain::createAddressMatcher(runtime.getActorVersion());
    OUTCOME_TRY(runtime.validateImmediateCallerType(
        address_matcher->getStorageMinerCodeId()));

    REQUIRE_NO_ERROR_A(state,
                       runtime.getActorState<MarketActorStatePtr>(),
                       VMExitCode::kErrIllegalState);

    // Lotus gas conformance
    REQUIRE_NO_ERROR(state->proposals.amt.loadRoot(),
                     VMExitCode::kErrIllegalState);

    const auto utils = Toolchain::createMarketUtils(runtime);

    std::vector<SectorWeights> weights;
    for (const auto &sector : params.sectors) {
      REQUIRE_NO_ERROR_A(
          result,
          utils->validateAndComputeDealWeight(
              state->proposals, sector.deal_ids, sector.sector_expiry),
          VMExitCode::kErrIllegalState);
      const auto &[deal_weight, verified_deal_weight, deal_space] = result;

      weights.push_back({.deal_space = deal_space,
                         .deal_weight = deal_weight,
                         .verified_deal_weight = verified_deal_weight});
    }

    return Result{weights};
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
