/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/impl/invoker_impl.hpp"

#include "vm/actor/builtin/v0/account/account_actor.hpp"
#include "vm/actor/builtin/v0/cron/cron_actor.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"
#include "vm/actor/builtin/v0/system/system_actor.hpp"
#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor.hpp"

#include "vm/actor/builtin/v2/account/account_actor.hpp"
#include "vm/actor/builtin/v2/cron/cron_actor.hpp"
#include "vm/actor/builtin/v2/init/init_actor.hpp"
#include "vm/actor/builtin/v2/market/market_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor.hpp"
#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor.hpp"
#include "vm/actor/builtin/v2/system/system_actor.hpp"
#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor {
  using runtime::InvocationOutput;

  InvokerImpl::InvokerImpl() {
    // v0
    builtin_[builtin::v0::kAccountCodeId] = builtin::v0::account::exports;
    builtin_[builtin::v0::kCronCodeId] = builtin::v0::cron::exports;
    builtin_[builtin::v0::kInitCodeId] = builtin::v0::init::exports;
    builtin_[builtin::v0::kStorageMarketCodeId] = builtin::v0::market::exports;
    builtin_[builtin::v0::kStorageMinerCodeId] = builtin::v0::miner::exports;
    builtin_[builtin::v0::kMultisigCodeId] = builtin::v0::multisig::exports;
    builtin_[builtin::v0::kPaymentChannelCodeId] =
        builtin::v0::payment_channel::exports;
    builtin_[builtin::v0::kRewardActorCodeId] = builtin::v0::reward::exports;
    builtin_[builtin::v0::kStoragePowerCodeId] =
        builtin::v0::storage_power::exports;
    builtin_[builtin::v0::kSystemActorCodeId] = builtin::v0::system::exports;
    builtin_[builtin::v0::kVerifiedRegistryCodeId] =
        builtin::v0::verified_registry::exports;

    // v2
    builtin_[builtin::v2::kAccountCodeId] = builtin::v2::account::exports;
    builtin_[builtin::v2::kCronCodeId] = builtin::v2::cron::exports;
    builtin_[builtin::v2::kInitCodeId] = builtin::v2::init::exports;
    builtin_[builtin::v2::kStorageMarketCodeId] = builtin::v2::market::exports;
    builtin_[builtin::v2::kStorageMinerCodeId] = builtin::v2::miner::exports;
    builtin_[builtin::v2::kRewardActorCodeId] = builtin::v2::reward::exports;
    builtin_[builtin::v2::kMultisigCodeId] = builtin::v2::multisig::exports;
    builtin_[builtin::v2::kPaymentChannelCodeId] =
        builtin::v2::payment_channel::exports;
    builtin_[builtin::v2::kStoragePowerCodeId] =
        builtin::v2::storage_power::exports;
    builtin_[builtin::v2::kSystemActorCodeId] = builtin::v2::system::exports;
    builtin_[builtin::v2::kVerifiedRegistryCodeId] =
        builtin::v2::verified_registry::exports;

    // Temp for miner actor
    ready_miner_actor_methods_v0.insert(builtin::v0::miner::Construct::Number);
    ready_miner_actor_methods_v0.insert(
        builtin::v0::miner::ControlAddresses::Number);
    ready_miner_actor_methods_v0.insert(
        builtin::v0::miner::ChangePeerId::Number);
    ready_miner_actor_methods_v0.insert(
        builtin::v0::miner::ChangeWorkerAddress::Number);

    ready_miner_actor_methods_v2.insert(builtin::v2::miner::Construct::Number);
    ready_miner_actor_methods_v2.insert(
        builtin::v2::miner::ControlAddresses::Number);
    ready_miner_actor_methods_v2.insert(
        builtin::v2::miner::ChangePeerId::Number);
    ready_miner_actor_methods_v2.insert(
        builtin::v2::miner::ChangeWorkerAddress::Number);
  }

  outcome::result<InvocationOutput> InvokerImpl::invoke(
      const Actor &actor, const std::shared_ptr<Runtime> &runtime) {
    // Temp for miner actor
    if (actor.code == builtin::v0::kStorageMinerCodeId) {
      if (ready_miner_actor_methods_v0.find(runtime->getMessage().get().method)
          == ready_miner_actor_methods_v0.end()) {
        return ::fc::vm::actor::cgo::invoke(actor.code, runtime);
      }
    }
    if (actor.code == builtin::v2::kStorageMinerCodeId) {
      if (ready_miner_actor_methods_v2.find(runtime->getMessage().get().method)
          == ready_miner_actor_methods_v2.end()) {
        return ::fc::vm::actor::cgo::invoke(actor.code, runtime);
      }
    }

    // TODO (a.chernyshov) remove after all cpp actors are implemented
    if (
        // v0
        (actor.code != builtin::v0::kAccountCodeId)            // < tested OK
        && (actor.code != builtin::v0::kCronCodeId)            // < tested OK
        && (actor.code != builtin::v0::kInitCodeId)            // < tested OK
        && (actor.code != builtin::v0::kStorageMarketCodeId)   // < tested OK
        && (actor.code != builtin::v0::kStorageMinerCodeId)    // WiP
        && (actor.code != builtin::v0::kMultisigCodeId)        // < tested OK
        && (actor.code != builtin::v0::kPaymentChannelCodeId)  // < tested OK
        && (actor.code != builtin::v0::kStoragePowerCodeId)    // < tested OK
        && (actor.code != builtin::v0::kRewardActorCodeId)     // < tested OK
        && (actor.code != builtin::v0::kSystemActorCodeId)     // < tested OK
        && (actor.code
            != builtin::v0::kVerifiedRegistryCodeId)  // < OK, but not tested

        // v2
        && (actor.code != builtin::v2::kAccountCodeId)  // < tested OK
        && (actor.code != builtin::v2::kCronCodeId)     // < tested OK
        && (actor.code != builtin::v2::kInitCodeId)     // < tested OK
        && (actor.code
            != builtin::v2::kStorageMarketCodeId)  // < OK, but not tested
        && (actor.code != builtin::v2::kStorageMinerCodeId)    // WiP
        && (actor.code != builtin::v2::kMultisigCodeId)        // < tested OK
        && (actor.code != builtin::v2::kPaymentChannelCodeId)  // < tested OK
        && (actor.code != builtin::v2::kStoragePowerCodeId)    // < tested OK
        && (actor.code != builtin::v2::kRewardActorCodeId)     // < tested OK
        && (actor.code != builtin::v2::kSystemActorCodeId)     // < tested OK
        && (actor.code
            != builtin::v2::kVerifiedRegistryCodeId)  // < OK, but not tested
    ) {
      return ::fc::vm::actor::cgo::invoke(actor.code, runtime);
    }

    auto maybe_builtin_actor = builtin_.find(actor.code);
    if (maybe_builtin_actor == builtin_.end()) {
      return VMExitCode::kSysErrIllegalActor;
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
