/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"

namespace fc::vm::actor::builtin::v2::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;

  constexpr int kSignersMax = 256;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::vector<Address> signers;
      size_t threshold{};
      EpochDuration unlock_duration{};
      ChainEpoch start_epoch{};
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(
      Construct::Params, signers, threshold, unlock_duration, start_epoch)

  using Propose = v0::multisig::Propose;
  using Approve = v0::multisig::Approve;
  using Cancel = v0::multisig::Cancel;

  struct AddSigner : ActorMethodBase<5> {
    using Params = v0::multisig::AddSigner::Params;

    ACTOR_METHOD_DECL();

    static outcome::result<void> checkSignersCount(
        const std::vector<Address> &signers);
  };

  struct RemoveSigner : ActorMethodBase<6> {
    using Params = v0::multisig::RemoveSigner::Params;

    ACTOR_METHOD_DECL();
  };

  struct SwapSigner : ActorMethodBase<7> {
    using Params = v0::multisig::SwapSigner::Params;

    ACTOR_METHOD_DECL();
  };

  using ChangeThreshold = v0::multisig::ChangeThreshold;

  struct LockBalance : ActorMethodBase<9> {
    using Params = v0::multisig::LockBalance::Params;

    ACTOR_METHOD_DECL();
  };

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::multisig
