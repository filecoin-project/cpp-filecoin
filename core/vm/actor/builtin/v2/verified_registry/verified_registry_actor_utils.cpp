/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/types/verified_registry/policy.hpp"

namespace fc::vm::actor::builtin::v2::verified_registry {
  using types::verified_registry::kMinVerifiedDealSize;

  outcome::result<void> VerifRegUtils::checkDealSize(
      const StoragePower &deal_size) const {
    if (deal_size < kMinVerifiedDealSize) {
      ABORT(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }

}  // namespace fc::vm::actor::builtin::v2::verified_registry
