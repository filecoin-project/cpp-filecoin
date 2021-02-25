/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/init/init_actor.hpp"

namespace fc::vm::actor::builtin::v3::init {

  using Construct = v2::init::Construct;
  using Exec = v2::init::Exec;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::init
