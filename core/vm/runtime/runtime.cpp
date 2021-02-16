/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/runtime.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::vm::runtime {
  using toolchain::Toolchain;

  outcome::result<void> Runtime::validateImmediateCallerIsSignable() const {
    OUTCOME_TRY(code, getActorCodeID(getImmediateCaller()));

    const auto address_matcher =
        Toolchain::createAddressMatcher(getActorVersion());

    if (address_matcher->isSignableActor(code)) {
      return outcome::success();
    }
    ABORT(VMExitCode::kSysErrForbidden);
  }

  outcome::result<void> Runtime::validateImmediateCallerIsMiner() const {
    OUTCOME_TRY(actual_code, getActorCodeID(getImmediateCaller()));

    const auto address_matcher =
        Toolchain::createAddressMatcher(getActorVersion());

    if (address_matcher->isStorageMinerActor(actual_code)) {
      return outcome::success();
    }
    ABORT(VMExitCode::kSysErrForbidden);
  }
}  // namespace fc::vm::runtime
