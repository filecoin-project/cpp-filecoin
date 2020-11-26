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
#include "vm/actor/builtin/v0/system/system_actor.hpp"
#include "vm/actor/cgo/actors.hpp"

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
    builtin_[builtin::v0::kSystemActorCodeID] = builtin::system::exports;
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
    if ((actor.code != builtin::v0::kAccountCodeCid)            // < tested OK
        && (actor.code != builtin::v0::kCronCodeCid)            // < tested OK
        && (actor.code != builtin::v0::kInitCodeCid)            // < tested OK
        && (actor.code != builtin::v0::kStorageMarketCodeCid)   // < tested OK
//        && (actor.code != builtin::v0::kStorageMinerCodeCid)    // TODO
//        && (actor.code != builtin::v0::kMultisigCodeCid)        // TODO
//        && (actor.code != builtin::v0::kPaymentChannelCodeCid)  // TODO
//        && (actor.code != builtin::v0::kStoragePowerCodeCid)    // < WiP
//        && (actor.code != builtin::v0::kRewardActorCodeID)      // TODO
        && (actor.code != builtin::v0::kSystemActorCodeID)      // < tested OK
//        && (actor.code != builtin::v0::kVerifiedRegistryCode)   // TODO
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
