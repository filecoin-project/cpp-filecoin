/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "testutil/outcome.hpp"
#include "vm/actor/builtin/states/state_manager.hpp"

namespace fc::vm::actor::builtin::states {
  class MockStateManager : public StateManager {
   public:
    MOCK_CONST_METHOD1(createAccountActorState,
                       AccountActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getAccountActorState,
                       outcome::result<AccountActorStatePtr>());

    MOCK_CONST_METHOD1(createCronActorState,
                       CronActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getCronActorState, outcome::result<CronActorStatePtr>());

    MOCK_CONST_METHOD1(createInitActorState,
                       InitActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getInitActorState, outcome::result<InitActorStatePtr>());

    MOCK_CONST_METHOD1(createMarketActorState,
                       MarketActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getMarketActorState,
                       outcome::result<MarketActorStatePtr>());

    MOCK_CONST_METHOD1(createMultisigActorState,
                       MultisigActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getMultisigActorState,
                       outcome::result<MultisigActorStatePtr>());

    MOCK_CONST_METHOD1(createPaymentChannelActorState,
                       PaymentChannelActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getPaymentChannelActorState,
                       outcome::result<PaymentChannelActorStatePtr>());

    MOCK_CONST_METHOD1(createPowerActorState,
                       PowerActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getPowerActorState,
                       outcome::result<PowerActorStatePtr>());

    MOCK_CONST_METHOD1(createRewardActorState,
                       RewardActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getRewardActorState,
                       outcome::result<RewardActorStatePtr>());

    MOCK_CONST_METHOD1(createSystemActorState,
                       SystemActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getSystemActorState,
                       outcome::result<SystemActorStatePtr>());

    MOCK_CONST_METHOD1(createVerifiedRegistryActorState,
                       VerifiedRegistryActorStatePtr(ActorVersion version));
    MOCK_CONST_METHOD0(getVerifiedRegistryActorState,
                       outcome::result<VerifiedRegistryActorStatePtr>());

    MOCK_METHOD1(commitState,
                 outcome::result<void>(const std::shared_ptr<State> &state));
  };
}  // namespace fc::vm::actor::builtin::states