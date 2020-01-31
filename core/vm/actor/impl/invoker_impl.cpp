/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/cron_actor.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    builtin_[actor::kCronCodeCid] = actor::cron_actor::exports;
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor,
      Runtime &runtime,
      MethodNumber method,
      const MethodParams &params) {
    if (actor.code == actor::kAccountCodeCid) {
      return CANT_INVOKE_ACCOUNT_ACTOR;
    }
    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return NO_CODE_OR_METHOD;
    }
    auto builtin_actor = maybe_builtin_actor->second;
    auto maybe_builtin_method = builtin_actor.find(method);
    if (maybe_builtin_method == builtin_actor.end()) {
      return NO_CODE_OR_METHOD;
    }
    return maybe_builtin_method->second(actor, runtime, params);
  }
}  // namespace fc::vm::actor
