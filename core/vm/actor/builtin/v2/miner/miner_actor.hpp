/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::TokenAmount;

  using Construct = v0::miner::Construct;
  using OnDeferredCronEvent = v0::miner::OnDeferredCronEvent;
  using ConfirmSectorProofsValid = v0::miner::ConfirmSectorProofsValid;

  struct ApplyRewards : ActorMethodBase<14> {
    struct Params {
      TokenAmount reward;
      TokenAmount penalty;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ApplyRewards::Params, reward, penalty)

}  // namespace fc::vm::actor::builtin::v2::miner
