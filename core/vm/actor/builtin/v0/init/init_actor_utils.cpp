/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::init {
  outcome::result<void> InitUtils::assertCaller(bool condition) const {
    return getRuntime().vm_assert(condition);
  }
}  // namespace fc::vm::actor::builtin::v0::init
