/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/multisig/multisig_actor.hpp"
#include "vm/actor/init_actor.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    builtin_[actor::kCronCodeCid] = actor::builtin::cron_actor::exports;
    builtin_[actor::kInitCodeCid] = actor::init_actor::exports;
    builtin_[actor::kMultisigCodeCid] = actor::builtin::multisig::exports;
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor,
      Runtime &runtime,
      MethodNumber method,
      const MethodParams &params) {
    if (actor.code == actor::kAccountCodeCid) {
      return VMExitCode::INVOKER_CANT_INVOKE_ACCOUNT_ACTOR;
    }
    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return VMExitCode::INVOKER_NO_CODE_OR_METHOD;
    }
    auto builtin_actor = maybe_builtin_actor->second;
    auto maybe_builtin_method = builtin_actor.find(method);
    if (maybe_builtin_method == builtin_actor.end()) {
      return VMExitCode::INVOKER_NO_CODE_OR_METHOD;
    }
    return maybe_builtin_method->second(actor, runtime, params);
  }
}  // namespace fc::vm::actor
