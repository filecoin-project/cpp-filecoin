/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  using primitives::ChainEpoch;
  using primitives::EpochDuration;

  using Construct = v2::multisig::Construct;
  using Propose = v2::multisig::Propose;
  using Approve = v2::multisig::Approve;
  using Cancel = v2::multisig::Cancel;
  using AddSigner = v2::multisig::AddSigner;
  using RemoveSigner = v2::multisig::RemoveSigner;
  using SwapSigner = v2::multisig::SwapSigner;
  using ChangeThreshold = v2::multisig::ChangeThreshold;

  struct LockBalance : ActorMethodBase<9> {
    using Params = v2::multisig::LockBalance::Params;

    ACTOR_METHOD_DECL();
  };

  /** Exported Multisig Actor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::multisig
