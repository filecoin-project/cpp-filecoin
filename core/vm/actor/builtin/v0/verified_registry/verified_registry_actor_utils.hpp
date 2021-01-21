/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_state.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils::verified_registry {
  using primitives::StoragePower;
  using primitives::address::Address;
  using v0::verified_registry::kMinVerifiedDealSize;

  outcome::result<void> checkDealSize(const StoragePower &deal_size);

  template <typename State>
  outcome::result<void> checkAddress(const State &state,
                                     const Address &address) {
    if (state.root_key == address) {
      return ABORT_CAST(VMExitCode::kErrIllegalArgument);
    }
    return outcome::success();
  }
}  // namespace fc::vm::actor::builtin::utils::verified_registry
