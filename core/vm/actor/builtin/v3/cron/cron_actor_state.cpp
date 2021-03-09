/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/cron/cron_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::cron {
  outcome::result<Buffer> CronActorState::toCbor() const {
    return Ipld::encode(*this);
  }
}  // namespace fc::vm::actor::builtin::v3::cron
