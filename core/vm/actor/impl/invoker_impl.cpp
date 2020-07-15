/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/builtin/account/account_actor.hpp"
#include "vm/actor/builtin/cron/cron_actor.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/actor/builtin/market/actor.hpp"
#include "vm/actor/builtin/miner/miner_actor.hpp"
#include "vm/actor/builtin/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/reward/reward_actor.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    builtin_[kInitCodeCid] = builtin::init::exports;
    builtin_[kRewardActorCodeID] = builtin::reward::exports;
    builtin_[kCronCodeCid] = builtin::cron::exports;
    builtin_[kStoragePowerCodeCid] = builtin::storage_power::exports;
    builtin_[kStorageMarketCodeCid] = builtin::market::exports;
    builtin_[kStorageMinerCodeCid] = builtin::miner::exports;
    builtin_[kMultisigCodeCid] = builtin::multisig::exports;
    builtin_[kPaymentChannelCodeCid] = builtin::payment_channel::exports;
    builtin_[kAccountCodeCid] = builtin::account::exports;
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor,
      Runtime &runtime,
      MethodNumber method,
      const MethodParams &params) {
    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return VMExitCode::kSysErrorIllegalActor;
    }
    auto builtin_actor = maybe_builtin_actor->second;
    auto maybe_builtin_method = builtin_actor.find(method);
    if (maybe_builtin_method == builtin_actor.end()) {
      return VMExitCode::kSysErrInvalidMethod;
    }
    return maybe_builtin_method->second(runtime, params);
  }
}  // namespace fc::vm::actor
