/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_MULTISIG_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_MULTISIG_ACTOR_HPP

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_utils.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using utils::multisig::MultisigUtils;

  struct Construct : ActorMethodBase<1> {
    using Params = v2::multisig::Construct::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };
  CBOR_TUPLE(
      Construct::Params, signers, threshold, unlock_duration, start_epoch)

  struct Propose : ActorMethodBase<2> {
    using Params = v2::multisig::Propose::Params;
    using Result = v2::multisig::Propose::Result;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct Approve : ActorMethodBase<3> {
    using Params = v2::multisig::Approve::Params;
    using Result = v2::multisig::Approve::Result;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct Cancel : ActorMethodBase<4> {
    using Params = v2::multisig::Cancel::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct AddSigner : ActorMethodBase<5> {
    using Params = v2::multisig::AddSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct RemoveSigner : ActorMethodBase<6> {
    using Params = v2::multisig::RemoveSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct SwapSigner : ActorMethodBase<7> {
    using Params = v2::multisig::SwapSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct ChangeThreshold : ActorMethodBase<8> {
    using Params = v2::multisig::ChangeThreshold::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct LockBalance : ActorMethodBase<9> {
    using Params = v2::multisig::LockBalance::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> checkAmount(const Runtime &runtime,
                                             const Params &params);
  };

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::multisig

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_V3_MULTISIG_ACTOR_HPP
