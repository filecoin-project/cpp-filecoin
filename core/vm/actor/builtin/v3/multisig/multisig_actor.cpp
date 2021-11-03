/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor.hpp"

#include "vm/actor/builtin/states/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using states::MultisigActorStatePtr;

  ACTOR_METHOD_IMPL(LockBalance) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    VALIDATE_ARG(params.unlock_duration > 0);
    VALIDATE_ARG(params.amount >= 0);
    OUTCOME_TRY(state, runtime.getActorState<MultisigActorStatePtr>());
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
