/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  outcome::result<Address> PaymentChannelUtils::resolveAccount(
      const Address &address, const CodeId &accountCodeCid) const {
    CHANGE_ERROR_A(
        resolved, getRuntime().resolveAddress(address), VMExitCode::kErrNotFound);

    CHANGE_ERROR_A(
        code, getRuntime().getActorCodeID(resolved), VMExitCode::kErrForbidden);
    if (code != accountCodeCid) {
      return VMExitCode::kErrForbidden;
    }
    return std::move(resolved);
  }
}  // namespace fc::vm::actor::builtin::v0::payment_channel
