/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using common::Buffer;
  using primitives::BigInt;
  using primitives::ChainEpoch;

  ACTOR_METHOD_IMPL(LockBalance) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(runtime.validateArgument(params.unlock_duration > 0));
    OUTCOME_TRY(runtime.validateArgument(params.amount >= 0));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::LockBalance::lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Propose>(),
      exportMethod<Approve>(),
      exportMethod<Cancel>(),
      exportMethod<AddSigner>(),
      exportMethod<RemoveSigner>(),
      exportMethod<SwapSigner>(),
      exportMethod<ChangeThreshold>(),
      exportMethod<LockBalance>(),
  };
}  // namespace fc::vm::actor::builtin::v3::multisig
