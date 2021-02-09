/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/init/init_actor.hpp"

namespace fc::vm::actor::builtin::v2::init {

  using InitActorState = v0::init::InitActorState;
  using Construct = v0::init::Construct;

  struct Exec : ActorMethodBase<2> {
    using Params = v0::init::Exec::Params;
    using Result = v0::init::Exec::Result;

    ACTOR_METHOD_DECL();

    using CallerAssert = v0::init::Exec::CallerAssert;
    using ExecAssert = v0::init::Exec::ExecAssert;

    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           CallerAssert caller_assert,
                                           ExecAssert exec_assert);
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::init
