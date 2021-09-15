/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/verified_registry/verified_registry_actor_state.hpp"
#include "vm/actor/builtin/utils/actor_utils.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::StoragePower;
  using primitives::address::Address;
  using runtime::Runtime;
  using states::VerifiedRegistryActorStatePtr;

  class VerifRegUtils : public ActorUtils {
   public:
    explicit VerifRegUtils(Runtime &r) : ActorUtils(r) {}

    virtual outcome::result<void> checkDealSize(
        const StoragePower &deal_size) const = 0;

    static inline outcome::result<void> checkAddress(
        const VerifiedRegistryActorStatePtr &state, const Address &address) {
      if (state->root_key == address) {
        ABORT(VMExitCode::kErrIllegalArgument);
      }
      return outcome::success();
    }

    virtual outcome::result<void> assertCap(bool condition) const = 0;
  };

  using VerifRegUtilsPtr = std::shared_ptr<VerifRegUtils>;

}  // namespace fc::vm::actor::builtin::utils
