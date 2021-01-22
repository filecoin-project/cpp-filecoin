/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_utils.hpp"

namespace fc::vm::actor::builtin::utils::verified_registry {

  outcome::result<void> checkDealSize(const StoragePower &deal_size) {
    if (deal_size < kMinVerifiedDealSize) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::utils::verified_registry
