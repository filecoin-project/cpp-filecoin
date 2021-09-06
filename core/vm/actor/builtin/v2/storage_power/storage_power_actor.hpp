/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {

  using Construct = v0::storage_power::Construct;

  struct CreateMiner : ActorMethodBase<2> {
    using Params = v0::storage_power::CreateMiner::Params;
    using Result = v0::storage_power::CreateMiner::Result;

    ACTOR_METHOD_DECL();
  };

  using UpdateClaimedPower = v0::storage_power::UpdateClaimedPower;
  using EnrollCronEvent = v0::storage_power::EnrollCronEvent;

  struct OnEpochTickEnd : ActorMethodBase<5> {
    ACTOR_METHOD_DECL();
  };

  using UpdatePledgeTotal = v0::storage_power::UpdatePledgeTotal;
  // method number 7 (OnConsensusFault) deprecated
  using SubmitPoRepForBulkVerify = v0::storage_power::SubmitPoRepForBulkVerify;
  using CurrentTotalPower = v0::storage_power::CurrentTotalPower;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::storage_power
