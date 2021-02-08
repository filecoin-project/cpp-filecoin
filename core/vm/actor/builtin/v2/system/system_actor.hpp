/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/system/system_actor.hpp"

namespace fc::vm::actor::builtin::v2::system {

  /**
   * System actor v2 is identical to System actor v0
   */
  using State = v0::system::State;
  using Construct = v0::system::Construct;

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v2::system
