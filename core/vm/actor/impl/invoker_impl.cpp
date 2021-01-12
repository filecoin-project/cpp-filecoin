/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/market/actor.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor {

  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    builtin_[builtin::v0::kInitCodeCid] = builtin::v0::init::exports;
    builtin_[builtin::v0::kRewardActorCodeID] = builtin::v0::reward::exports;
    builtin_[builtin::v0::kCronCodeCid] = builtin::v0::cron::exports;
    builtin_[builtin::v0::kStoragePowerCodeCid] =
        builtin::v0::storage_power::exports;
    builtin_[builtin::v0::kStorageMarketCodeCid] = builtin::v0::market::exports;
    builtin_[builtin::v0::kStorageMinerCodeCid] = builtin::v0::miner::exports;
    builtin_[builtin::v0::kMultisigCodeCid] = builtin::v0::multisig::exports;
    builtin_[builtin::v0::kPaymentChannelCodeCid] =
        builtin::v0::payment_channel::exports;
    builtin_[builtin::v0::kAccountCodeCid] = builtin::v0::account::exports;
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
