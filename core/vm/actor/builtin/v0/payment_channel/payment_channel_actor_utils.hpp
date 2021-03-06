/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/payment_channel_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::payment_channel {
  using primitives::address::Address;
  using runtime::Runtime;

  class PaymentChannelUtils : public utils::PaymentChannelUtils {
   public:
    explicit PaymentChannelUtils(Runtime &r) : utils::PaymentChannelUtils(r) {}

    outcome::result<Address> resolveAccount(
        const Address &address, const CodeId &accountCodeCid) const override;
  };
}  // namespace fc::vm::actor::builtin::v0::payment_channel
