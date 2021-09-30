/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/reward/reward_actor.hpp"

namespace fc::vm::actor::builtin::v3::reward {

  // TODO (m.tagirov) implement v3
  using Constructor = v2::reward::Constructor;
  using AwardBlockReward = v2::reward::AwardBlockReward;
  using ThisEpochReward = v2::reward::ThisEpochReward;
  using UpdateNetworkKPI = v2::reward::UpdateNetworkKPI;

}  // namespace fc::vm::actor::builtin::v3::reward
