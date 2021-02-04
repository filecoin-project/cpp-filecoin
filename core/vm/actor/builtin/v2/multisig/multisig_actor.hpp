/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_utils.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using utils::multisig::MultisigUtils;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold{};
      EpochDuration unlock_duration{};
      ChainEpoch start_epoch{};
    };

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static void setLocked(const Runtime &runtime,
                          const ChainEpoch &start_epoch,
                          const EpochDuration &unlock_duration,
                          State &state);
  };
  CBOR_TUPLE(
      Construct::Params, signers, threshold, unlock_duration, start_epoch)

  struct Propose : ActorMethodBase<2> {
    using Params = v0::multisig::Propose::Params;
    using Result = v0::multisig::Propose::Result;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct Approve : ActorMethodBase<3> {
    using Params = v0::multisig::Approve::Params;
    using Result = v0::multisig::Approve::Result;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct Cancel : ActorMethodBase<4> {
    using Params = v0::multisig::Cancel::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct AddSigner : ActorMethodBase<5> {
    using Params = v0::multisig::AddSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> checkSignersCount(
        const std::vector<Address> &signers);
  };

  struct RemoveSigner : ActorMethodBase<6> {
    using Params = v0::multisig::RemoveSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);

    static outcome::result<void> removeSigner(const Params &params,
                                              State &state,
                                              const Address &signer);
  };

  struct SwapSigner : ActorMethodBase<7> {
    using Params = v0::multisig::SwapSigner::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct ChangeThreshold : ActorMethodBase<8> {
    using Params = v0::multisig::ChangeThreshold::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  struct LockBalance : ActorMethodBase<9> {
    using Params = v0::multisig::LockBalance::Params;

    ACTOR_METHOD_DECL();
    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           const MultisigUtils &utils);
  };

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::multisig
