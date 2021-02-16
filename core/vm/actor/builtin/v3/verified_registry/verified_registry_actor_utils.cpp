/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::verified_registry {

  outcome::result<void> VerifRegUtils::checkDealSize(
      const StoragePower &deal_size) const {
    if (deal_size < kMinVerifiedDealSize) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

  outcome::result<void> VerifRegUtils::assertCap(bool condition) const {
    return runtime.requireState(condition);
  }

}  // namespace fc::vm::actor::builtin::v3::verified_registry
