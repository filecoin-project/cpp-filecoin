/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v3::storage_power {

  using Construct = v2::storage_power::Construct;
  using CreateMiner = v2::storage_power::CreateMiner;
  using UpdateClaimedPower = v2::storage_power::UpdateClaimedPower;
  using EnrollCronEvent = v2::storage_power::EnrollCronEvent;
  using OnEpochTickEnd = v2::storage_power::OnEpochTickEnd;
  using UpdatePledgeTotal = v2::storage_power::UpdatePledgeTotal;
  using SubmitPoRepForBulkVerify = v2::storage_power::SubmitPoRepForBulkVerify;
  using CurrentTotalPower = v2::storage_power::CurrentTotalPower;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::storage_power
