/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v3/multisig/impl/multisig_utils_impl_v3.hpp"

namespace fc::vm::actor::builtin::v3::multisig {

  using fc::common::Buffer;
  using fc::primitives::BigInt;
  using fc::primitives::ChainEpoch;
  using fc::vm::VMExitCode;
  using fc::vm::actor::ActorExports;
  using fc::vm::actor::ActorMethod;
  using fc::vm::actor::kInitAddress;
  using fc::vm::runtime::InvocationOutput;
  using fc::vm::runtime::Runtime;
  namespace outcome = fc::outcome;

  // Propose
  //============================================================================

  outcome::result<Propose::Result> Propose::execute(
      Runtime &runtime,
      const Propose::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::Propose::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Propose) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // Approve
  //============================================================================

  outcome::result<Approve::Result> Approve::execute(
      Runtime &runtime,
      const Approve::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::Approve::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Approve) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // Cancel
  //============================================================================

  outcome::result<Cancel::Result> Cancel::execute(Runtime &runtime,
                                                  const Cancel::Params &params,
                                                  const MultisigUtils &utils) {
    return v2::multisig::Cancel::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(Cancel) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // AddSigner
  //============================================================================

  outcome::result<AddSigner::Result> AddSigner::execute(
      Runtime &runtime,
      const AddSigner::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::AddSigner::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(AddSigner) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // RemoveSigner
  //============================================================================

  outcome::result<RemoveSigner::Result> RemoveSigner::execute(
      Runtime &runtime,
      const RemoveSigner::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::RemoveSigner::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(RemoveSigner) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // SwapSigner
  //============================================================================

  outcome::result<SwapSigner::Result> SwapSigner::execute(
      Runtime &runtime,
      const SwapSigner::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::SwapSigner::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(SwapSigner) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // ChangeThreshold
  //============================================================================

  outcome::result<ChangeThreshold::Result> ChangeThreshold::execute(
      Runtime &runtime,
      const ChangeThreshold::Params &params,
      const MultisigUtils &utils) {
    return v2::multisig::ChangeThreshold::execute(runtime, params, utils);
  }

  ACTOR_METHOD_IMPL(ChangeThreshold) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  // LockBalance
  //============================================================================

  outcome::result<LockBalance::Result> LockBalance::execute(
      Runtime &runtime,
      const LockBalance::Params &params,
      const MultisigUtils &utils) {
    OUTCOME_TRY(runtime.validateImmediateCallerIsCurrentReceiver());
    OUTCOME_TRY(runtime.validateArgument(params.unlock_duration > 0));
    OUTCOME_TRY(runtime.validateArgument(params.amount >= 0));
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(v0::multisig::LockBalance::lockBalance(params, state));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(LockBalance) {
    const MultisigUtilsImplV3 utils;
    return execute(runtime, params, utils);
  }

  //============================================================================

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
