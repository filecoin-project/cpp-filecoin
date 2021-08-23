/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v2/miner_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::sector::getRegisteredWindowPoStProof;

  outcome::result<Universal<MinerInfo>> MinerActorState::getInfo() const {
    OUTCOME_TRY(info, miner_info.get());
    OUTCOME_TRYA(info->window_post_proof_type,
                 getRegisteredWindowPoStProof(info->seal_proof_type));
    return std::move(info);
  }

}  // namespace fc::vm::actor::builtin::v2::miner
