/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::utils::payment_channel {
  using primitives::address::Address;
  using runtime::Runtime;

  class PaymentChannelUtils {
   public:
    explicit PaymentChannelUtils(Runtime &r) : runtime(r) {}
    virtual ~PaymentChannelUtils() = default;

    virtual outcome::result<Address> resolveAccount(
        const Address &address, const CodeId &accountCodeCid) const = 0;

   protected:
    Runtime &runtime;
  };

  using PaymentChannelUtilsPtr = std::shared_ptr<PaymentChannelUtils>;

}  // namespace fc::vm::actor::builtin::utils::payment_channel
