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
#include "vm/actor/builtin/system/system_actor.hpp"
#include "vm/actor/cgo/actors.hpp"

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
    builtin_[kSystemActorCodeID] = builtin::system::exports;
  }

  void InvokerImpl::config(
      const StoragePower &min_verified_deal_size,
      const StoragePower &consensus_miner_min_power,
      const std::vector<RegisteredProof> &supported_proofs) {
    // TODO (a.chernyshov) implement
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor, const std::shared_ptr<Runtime> &runtime) {
    // TODO (a.chernyshov) remove after all cpp actors are implemented
    if (
        // (actor.code == kAccountCodeCid) // < tested OK
        // || (actor.code == kCronCodeCid) // < tested OK
        // || (actor.code == kInitCodeCid) // < tested OK
        // || (actor.code == kStorageMarketCodeCid) // < tested OK
        (actor.code == kStorageMinerCodeCid)       //
        || (actor.code == kMultisigCodeCid)        //
        || (actor.code == kPaymentChannelCodeCid)  //
        // || (actor.code == kStoragePowerCodeCid) // < WiP
        || (actor.code == kRewardActorCodeID)  //
                                               // System
                                               // Verified Registry
    ) {
      auto message = runtime->getMessage();
      return ::fc::vm::actor::cgo::invoke(runtime->execution(),
                                          message,
                                          actor.code,
                                          message.get().method,
                                          message.get().params);
    }

    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return VMExitCode::kSysErrorIllegalActor;
    }
    auto builtin_actor = maybe_builtin_actor->second;
    auto message = runtime->getMessage();
    auto maybe_builtin_method = builtin_actor.find(message.get().method);
    if (maybe_builtin_method == builtin_actor.end()) {
      return VMExitCode::kSysErrInvalidMethod;
    }
    const auto &res =
        maybe_builtin_method->second(*runtime, message.get().params);
    return res;
  }
}  // namespace fc::vm::actor
