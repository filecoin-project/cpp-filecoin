/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/init/init_actor.hpp"

namespace fc::vm::actor::builtin::v3::init {

  using InitActorState = v2::init::InitActorState;
  using Construct = v2::init::Construct;

  struct Exec : ActorMethodBase<2> {
    using Params = v2::init::Exec::Params;
    using Result = v2::init::Exec::Result;

    ACTOR_METHOD_DECL();

    using CallerAssert = v2::init::Exec::CallerAssert;
    using ExecAssert = v2::init::Exec::ExecAssert;

    static outcome::result<Result> execute(Runtime &runtime,
                                           const Params &params,
                                           CallerAssert caller_assert,
                                           ExecAssert exec_assert);
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::init
