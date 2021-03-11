/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/init/init_actor.hpp"

namespace fc::vm::actor::builtin::v2::init {

  using Construct = v0::init::Construct;
  using Exec = v0::init::Exec;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::init
