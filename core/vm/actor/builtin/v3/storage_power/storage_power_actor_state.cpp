/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/storage_power/storage_power_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::storage_power {
  outcome::result<Buffer> PowerActorState::toCbor() const {
    return Ipld::encode(*this);
  }
}  // namespace fc::vm::actor::builtin::v3::storage_power
