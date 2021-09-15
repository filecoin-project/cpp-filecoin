/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/init/init_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::init {
  outcome::result<void> InitUtils::assertCaller(bool condition) const {
    return getRuntime().requireState(condition);
  }
}  // namespace fc::vm::actor::builtin::v3::init
