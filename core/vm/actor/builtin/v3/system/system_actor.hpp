/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/system/system_actor.hpp"

namespace fc::vm::actor::builtin::v3::system {

  /**
   * System actor v3 is identical to System actor v2
   */
  using State = v2::system::State;
  using Construct = v2::system::Construct;

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v3::system
