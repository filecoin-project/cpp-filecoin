/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    builtin_[actor::kAccountCodeCid] = actor::builtin::account::exports;
    builtin_[actor::kCronCodeCid] = actor::builtin::cron::exports;
    builtin_[actor::kInitCodeCid] = actor::builtin::init::exports;
    builtin_[actor::kMultisigCodeCid] = actor::builtin::multisig::exports;
    builtin_[actor::kStoragePowerCodeCid] =
        actor::builtin::storage_power::exports;
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor,
      Runtime &runtime,
      MethodNumber method,
      const MethodParams &params) {
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
