/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/actor_utils.hpp"

#include "primitives/address/address.hpp"

namespace fc::vm::actor::builtin::utils {
  using primitives::address::Address;

  class PaymentChannelUtils : public ActorUtils {
   public:
    explicit PaymentChannelUtils(Runtime &r) : ActorUtils(r) {}

    virtual outcome::result<Address> resolveAccount(
        const Address &address, const CodeId &accountCodeCid) const = 0;
  };

  using PaymentChannelUtilsPtr = std::shared_ptr<PaymentChannelUtils>;

}  // namespace fc::vm::actor::builtin::utils
