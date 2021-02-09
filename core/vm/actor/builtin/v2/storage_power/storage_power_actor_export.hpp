/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct CreateMiner : ActorMethodBase<2> {
    using Params = v0::storage_power::CreateMiner::Params;
    using Result = v0::storage_power::CreateMiner::Result;

    ACTOR_METHOD_DECL();
  };

  struct UpdateClaimedPower : ActorMethodBase<3> {
    using Params = v0::storage_power::UpdateClaimedPower::Params;

    ACTOR_METHOD_DECL();
  };

  struct EnrollCronEvent : ActorMethodBase<4> {
    using Params = v0::storage_power::EnrollCronEvent::Params;

    ACTOR_METHOD_DECL();
  };

  struct OnEpochTickEnd : ActorMethodBase<5> {
    ACTOR_METHOD_DECL();
  };

  struct UpdatePledgeTotal : ActorMethodBase<6> {
    using Params = v0::storage_power::UpdatePledgeTotal::Params;

    ACTOR_METHOD_DECL();
  };

  // method number 7 (OnConsensusFault) deprecated

  struct SubmitPoRepForBulkVerify : ActorMethodBase<8> {
    using Params = v0::storage_power::SubmitPoRepForBulkVerify::Params;

    ACTOR_METHOD_DECL();
  };

  struct CurrentTotalPower : ActorMethodBase<9> {
    using Result = v0::storage_power::CurrentTotalPower::Result;

    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::storage_power
