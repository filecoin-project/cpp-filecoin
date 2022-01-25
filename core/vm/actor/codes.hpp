/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/code.hpp"

namespace fc::vm::actor::builtin::v0 {
  constexpr ActorCodeCid kAccountCodeId{"fil/1/account"};
  constexpr ActorCodeCid kCronCodeId{"fil/1/cron"};
  constexpr ActorCodeCid kStoragePowerCodeId{"fil/1/storagepower"};
  constexpr ActorCodeCid kStorageMarketCodeId{"fil/1/storagemarket"};
  constexpr ActorCodeCid kStorageMinerCodeId{"fil/1/storageminer"};
  constexpr ActorCodeCid kMultisigCodeId{"fil/1/multisig"};
  constexpr ActorCodeCid kInitCodeId{"fil/1/init"};
  constexpr ActorCodeCid kPaymentChannelCodeId{"fil/1/paymentchannel"};
  constexpr ActorCodeCid kRewardActorCodeId{"fil/1/reward"};
  constexpr ActorCodeCid kSystemActorCodeId{"fil/1/system"};
  constexpr ActorCodeCid kVerifiedRegistryCodeId{"fil/1/verifiedregistry"};
};  // namespace fc::vm::actor::builtin::v0

#define VM_ACTOR_BUILTIN_V_CODE(V)                                             \
  namespace fc::vm::actor::builtin::v##V {                                     \
    constexpr ActorCodeCid kAccountCodeId{"fil/" #V "/account"};               \
    constexpr ActorCodeCid kCronCodeId{"fil/" #V "/cron"};                     \
    constexpr ActorCodeCid kStoragePowerCodeId{"fil/" #V "/storagepower"};     \
    constexpr ActorCodeCid kStorageMarketCodeId{"fil/" #V "/storagemarket"};   \
    constexpr ActorCodeCid kStorageMinerCodeId{"fil/" #V "/storageminer"};     \
    constexpr ActorCodeCid kMultisigCodeId{"fil/" #V "/multisig"};             \
    constexpr ActorCodeCid kInitCodeId{"fil/" #V "/init"};                     \
    constexpr ActorCodeCid kPaymentChannelCodeId{"fil/" #V "/paymentchannel"}; \
    constexpr ActorCodeCid kRewardActorCodeId{"fil/" #V "/reward"};            \
    constexpr ActorCodeCid kSystemActorCodeId{"fil/" #V "/system"};            \
    constexpr ActorCodeCid kVerifiedRegistryCodeId{"fil/" #V                   \
                                                   "/verifiedregistry"};       \
  }

VM_ACTOR_BUILTIN_V_CODE(2)
VM_ACTOR_BUILTIN_V_CODE(3)
VM_ACTOR_BUILTIN_V_CODE(4)
VM_ACTOR_BUILTIN_V_CODE(5)
VM_ACTOR_BUILTIN_V_CODE(6)
VM_ACTOR_BUILTIN_V_CODE(7)

#undef VM_ACTOR_BUILTIN_V_CODE
