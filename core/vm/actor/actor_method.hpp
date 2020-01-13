/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/vm_context.hpp"

namespace fc::vm::actor {
  using common::Buffer;

  using ActorMethod = std::function<outcome::result<Buffer>(
      const Actor &, VMContext &, gsl::span<const uint8_t>)>;

  using ActorExports = std::map<uint64_t, ActorMethod>;
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
