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
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"
#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v2/cron/cron_actor.hpp"
#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/builtin/v2/system/system_actor.hpp"
#include "vm/actor/cgo/actors.hpp"

namespace fc::vm::actor {
  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    // v0
    builtin_[builtin::v0::kAccountCodeCid] = builtin::v0::account::exports;
    builtin_[builtin::v0::kCronCodeCid] = builtin::v0::cron::exports;
    builtin_[builtin::v0::kInitCodeCid] = builtin::v0::init::exports;
    builtin_[builtin::v0::kStorageMarketCodeCid] = builtin::v0::market::exports;
    builtin_[builtin::v0::kStorageMinerCodeCid] = builtin::v0::miner::exports;
    builtin_[builtin::v0::kMultisigCodeCid] = builtin::v0::multisig::exports;
    builtin_[builtin::v0::kPaymentChannelCodeCid] =
        builtin::v0::payment_channel::exports;
    builtin_[builtin::v0::kRewardActorCodeID] = builtin::v0::reward::exports;
    builtin_[builtin::v0::kStoragePowerCodeCid] =
        builtin::v0::storage_power::exports;
    builtin_[builtin::v0::kSystemActorCodeID] = builtin::v0::system::exports;
    builtin_[builtin::v0::kVerifiedRegistryCode] = builtin::v0::verified_registry::exports;

    // v2
    builtin_[builtin::v2::kAccountCodeCid] = builtin::v2::account::exports;
    builtin_[builtin::v2::kCronCodeCid] = builtin::v2::cron::exports;
    // builtin_[builtin::v2::kInitCodeCid] = builtin::v2::init::exports;
    // builtin_[builtin::v2::kStorageMarketCodeCid] =
    // builtin::v2::market::exports;
    // builtin_[builtin::v2::kStorageMinerCodeCid]
    // = builtin::v2::miner::exports;
    // builtin_[builtin::v2::kRewardActorCodeID]
    // = builtin::v2::reward::exports;
    // builtin_[builtin::v2::kMultisigCodeCid] = builtin::v2::multisig::exports;
    builtin_[builtin::v2::kPaymentChannelCodeCid] =
        builtin::v2::payment_channel::exports;
    builtin_[builtin::v2::kStoragePowerCodeCid] =
        builtin::v2::storage_power::exports;
    builtin_[builtin::v2::kSystemActorCodeID] = builtin::v2::system::exports;
    // builtin_[builtin::v2::kVerifiedRegistryCode] = builtin::v2::verified_registry::exports;
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
        // v0
        (actor.code != builtin::v0::kAccountCodeCid)           // < tested OK
        && (actor.code != builtin::v0::kCronCodeCid)           // < tested OK
        && (actor.code != builtin::v0::kInitCodeCid)           // < tested OK
        && (actor.code != builtin::v0::kStorageMarketCodeCid)  // < tested OK
        // && (actor.code != builtin::v0::kStorageMinerCodeCid)    // TODO
        // && (actor.code != builtin::v0::kMultisigCodeCid)        // TODO
        && (actor.code != builtin::v0::kPaymentChannelCodeCid)  // < tested OK
        && (actor.code != builtin::v0::kStoragePowerCodeCid)    // < tested OK
        // && (actor.code != builtin::v0::kRewardActorCodeID)      // TODO
        && (actor.code != builtin::v0::kSystemActorCodeID)  // < tested OK
        && (actor.code != builtin::v0::kVerifiedRegistryCode)   // WiP

        // v2
        && (actor.code != builtin::v2::kAccountCodeCid)  // < tested OK
        && (actor.code != builtin::v2::kCronCodeCid)     // TODO
        // && (actor.code != builtin::v2::kInitCodeCid)            // TODO
        // && (actor.code != builtin::v2::kStorageMarketCodeCid)   // TODO
        // && (actor.code != builtin::v2::kStorageMinerCodeCid)    // TODO
        // && (actor.code != builtin::v2::kMultisigCodeCid)        // TODO
        && (actor.code != builtin::v2::kPaymentChannelCodeCid)  // < tested OK
        && (actor.code != builtin::v2::kStoragePowerCodeCid)    // < tested OK
        // && (actor.code != builtin::v2::kRewardActorCodeID)      // TODO
        && (actor.code != builtin::v2::kSystemActorCodeID)  // < tested OK
        // && (actor.code != builtin::v2::kVerifiedRegistryCode)   // TODO
    ) {
      return ::fc::vm::actor::cgo::invoke(actor.code, runtime);
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
