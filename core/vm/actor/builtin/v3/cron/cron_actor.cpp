/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/cron/cron_actor.hpp"

namespace fc::vm::actor::builtin::v3::cron {

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<EpochTick>(),
  };
}  // namespace fc::vm::actor::builtin::v3::cron
