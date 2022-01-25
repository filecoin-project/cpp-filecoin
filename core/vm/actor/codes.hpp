/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/code.hpp"

/**
 * Defines Actor Codes, V is an actor version and S is a string representation.
 * Note: V0 actor has "fil/1/..." string representation
 */
#define VM_ACTOR_BUILTIN_V_CODE(V, S)                                         \
  namespace fc::vm::actor::builtin::v##V {                                    \
    constexpr ActorCodeCid kAccountCodeId{"fil/" S "/account"};               \
    constexpr ActorCodeCid kCronCodeId{"fil/" S "/cron"};                     \
    constexpr ActorCodeCid kStoragePowerCodeId{"fil/" S "/storagepower"};     \
    constexpr ActorCodeCid kStorageMarketCodeId{"fil/" S "/storagemarket"};   \
    constexpr ActorCodeCid kStorageMinerCodeId{"fil/" S "/storageminer"};     \
    constexpr ActorCodeCid kMultisigCodeId{"fil/" S "/multisig"};             \
    constexpr ActorCodeCid kInitCodeId{"fil/" S "/init"};                     \
    constexpr ActorCodeCid kPaymentChannelCodeId{"fil/" S "/paymentchannel"}; \
    constexpr ActorCodeCid kRewardActorCodeId{"fil/" S "/reward"};            \
    constexpr ActorCodeCid kSystemActorCodeId{"fil/" S "/system"};            \
    constexpr ActorCodeCid kVerifiedRegistryCodeId{"fil/" S                   \
                                                   "/verifiedregistry"};      \
  }

VM_ACTOR_BUILTIN_V_CODE(0, "1")
VM_ACTOR_BUILTIN_V_CODE(2, "2")
VM_ACTOR_BUILTIN_V_CODE(3, "3")
VM_ACTOR_BUILTIN_V_CODE(4, "4")
VM_ACTOR_BUILTIN_V_CODE(5, "5")
VM_ACTOR_BUILTIN_V_CODE(6, "6")
VM_ACTOR_BUILTIN_V_CODE(7, "7")

#undef VM_ACTOR_BUILTIN_V_CODE
