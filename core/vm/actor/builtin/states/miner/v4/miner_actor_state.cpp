/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v4/miner_actor_state.hpp"

namespace fc::vm::actor::builtin::v4::miner {
  using namespace types::miner;

  outcome::result<Universal<MinerInfo>> MinerActorState::getInfo() const {
    return miner_info.get();
  }

}  // namespace fc::vm::actor::builtin::v4::miner
