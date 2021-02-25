/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {
  using primitives::address::Address;

  struct CronTableEntry {
    Address to_addr{};
    MethodNumber method_num{};
  };
  CBOR_TUPLE(CronTableEntry, to_addr, method_num)

  struct CronActorState : State {
    explicit CronActorState(ActorVersion version)
        : State(ActorType::kCron, version) {}

    std::vector<CronTableEntry> entries;
  };

  using CronActorStatePtr = std::shared_ptr<CronActorState>;
}  // namespace fc::vm::actor::builtin::states
