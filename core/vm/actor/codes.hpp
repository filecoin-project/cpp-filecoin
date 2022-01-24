/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/code.hpp"

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

VM_ACTOR_BUILTIN_V_CODE(0)
VM_ACTOR_BUILTIN_V_CODE(2)
VM_ACTOR_BUILTIN_V_CODE(3)
VM_ACTOR_BUILTIN_V_CODE(4)
VM_ACTOR_BUILTIN_V_CODE(5)
VM_ACTOR_BUILTIN_V_CODE(6)
VM_ACTOR_BUILTIN_V_CODE(7)

#undef VM_ACTOR_BUILTIN_V_CODE
