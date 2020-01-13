/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/invoker.hpp"

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor {
  Invoker::Invoker() {
    builtin_[actor::kCronCodeCid] = actor::CronActor::exports;
  }

  outcome::result<Buffer> Invoker::invoke(const Actor &actor,
                                          VMContext &context,
                                          uint64_t method,
                                          gsl::span<const uint8_t> params) {
    if (actor.code == actor::kAccountCodeCid) {
      return VMExitCode(254);
    }
    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return VMExitCode(255);
    }
    auto builtin_actor = maybe_builtin_actor->second;
    auto maybe_builtin_method = builtin_actor.find(method);
    if (maybe_builtin_method == builtin_actor.end()) {
      return VMExitCode(255);
    }
    return maybe_builtin_method->second(actor, context, params);
  }
}  // namespace fc::vm::actor
