/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/code.hpp"

namespace fc::vm::actor::code {
  using Code = ActorCodeCid;

  constexpr Code account0{"fil/1/account"};
  constexpr Code cron0{"fil/1/cron"};
  constexpr Code power0{"fil/1/storagepower"};
  constexpr Code market0{"fil/1/storagemarket"};
  constexpr Code miner0{"fil/1/storageminer"};
  constexpr Code multisig0{"fil/1/multisig"};
  constexpr Code init0{"fil/1/init"};
  constexpr Code paych0{"fil/1/paymentchannel"};
  constexpr Code reward0{"fil/1/reward"};
  constexpr Code system0{"fil/1/system"};
  constexpr Code verifreg0{"fil/1/verifiedregistry"};

  constexpr Code account2{"fil/2/account"};
  constexpr Code cron2{"fil/2/cron"};
  constexpr Code power2{"fil/2/storagepower"};
  constexpr Code market2{"fil/2/storagemarket"};
  constexpr Code miner2{"fil/2/storageminer"};
  constexpr Code multisig2{"fil/2/multisig"};
  constexpr Code init2{"fil/2/init"};
  constexpr Code paych2{"fil/2/paymentchannel"};
  constexpr Code reward2{"fil/2/reward"};
  constexpr Code system2{"fil/2/system"};
  constexpr Code verifreg2{"fil/2/verifiedregistry"};

  constexpr Code account3{"fil/3/account"};
  constexpr Code cron3{"fil/3/cron"};
  constexpr Code power3{"fil/3/storagepower"};
  constexpr Code market3{"fil/3/storagemarket"};
  constexpr Code miner3{"fil/3/storageminer"};
  constexpr Code multisig3{"fil/3/multisig"};
  constexpr Code init3{"fil/3/init"};
  constexpr Code paych3{"fil/3/paymentchannel"};
  constexpr Code reward3{"fil/3/reward"};
  constexpr Code system3{"fil/3/system"};
  constexpr Code verifreg3{"fil/3/verifiedregistry"};

  constexpr Code account4{"fil/4/account"};
  constexpr Code cron4{"fil/4/cron"};
  constexpr Code power4{"fil/4/storagepower"};
  constexpr Code market4{"fil/4/storagemarket"};
  constexpr Code miner4{"fil/4/storageminer"};
  constexpr Code multisig4{"fil/4/multisig"};
  constexpr Code init4{"fil/4/init"};
  constexpr Code paych4{"fil/4/paymentchannel"};
  constexpr Code reward4{"fil/4/reward"};
  constexpr Code system4{"fil/4/system"};
  constexpr Code verifreg4{"fil/4/verifiedregistry"};

  constexpr Code account5{"fil/5/account"};
  constexpr Code cron5{"fil/5/cron"};
  constexpr Code power5{"fil/5/storagepower"};
  constexpr Code market5{"fil/5/storagemarket"};
  constexpr Code miner5{"fil/5/storageminer"};
  constexpr Code multisig5{"fil/5/multisig"};
  constexpr Code init5{"fil/5/init"};
  constexpr Code paych5{"fil/5/paymentchannel"};
  constexpr Code reward5{"fil/5/reward"};
  constexpr Code system5{"fil/5/system"};
  constexpr Code verifreg5{"fil/5/verifiedregistry"};
}  // namespace fc::vm::actor::code

#define VM_ACTOR_BUILTIN_V_CODE(V)                             \
  namespace fc::vm::actor::builtin::v##V {                     \
    constexpr auto kAccountCodeId{code::account##V};           \
    constexpr auto kCronCodeId{code::cron##V};                 \
    constexpr auto kStoragePowerCodeId{code::power##V};        \
    constexpr auto kStorageMarketCodeId{code::market##V};      \
    constexpr auto kStorageMinerCodeId{code::miner##V};        \
    constexpr auto kMultisigCodeId{code::multisig##V};         \
    constexpr auto kInitCodeId{code::init##V};                 \
    constexpr auto kPaymentChannelCodeId{code::paych##V};      \
    constexpr auto kRewardActorCodeId{code::reward##V};        \
    constexpr auto kSystemActorCodeId{code::system##V};        \
    constexpr auto kVerifiedRegistryCodeId{code::verifreg##V}; \
  }
VM_ACTOR_BUILTIN_V_CODE(0)
VM_ACTOR_BUILTIN_V_CODE(2)
VM_ACTOR_BUILTIN_V_CODE(3)
VM_ACTOR_BUILTIN_V_CODE(4)
VM_ACTOR_BUILTIN_V_CODE(5)
#undef VM_ACTOR_BUILTIN_V_CODE
