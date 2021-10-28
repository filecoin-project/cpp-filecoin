/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::types::cron {
  using primitives::address::Address;

  struct CronTableEntry {
    Address to_addr;
    MethodNumber method_num{};
  };
  CBOR_TUPLE(CronTableEntry, to_addr, method_num)
}  // namespace fc::vm::actor::builtin::types::cron
