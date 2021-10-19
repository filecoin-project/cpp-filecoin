/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/utils/init_actor_utils.hpp"
#include "vm/actor/builtin/utils/market_actor_utils.hpp"
#include "vm/actor/builtin/utils/miner_actor_utils.hpp"
#include "vm/actor/builtin/utils/multisig_actor_utils.hpp"
#include "vm/actor/builtin/utils/payment_channel_actor_utils.hpp"
#include "vm/actor/builtin/utils/power_actor_utils.hpp"
#include "vm/actor/builtin/utils/verified_registry_actor_utils.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/toolchain/address_matcher.hpp"

namespace fc::vm::toolchain {
  using actor::Actor;
  using actor::ActorVersion;
  using actor::CodeId;
  using actor::builtin::utils::InitUtilsPtr;
  using actor::builtin::utils::MarketUtilsPtr;
  using actor::builtin::utils::MinerUtilsPtr;
  using actor::builtin::utils::MultisigUtilsPtr;
  using actor::builtin::utils::PaymentChannelUtilsPtr;
  using actor::builtin::utils::PowerUtilsPtr;
  using actor::builtin::utils::VerifRegUtilsPtr;
  using runtime::Runtime;
  using version::NetworkVersion;

  class Toolchain {
   public:
    /**
     * Returns actor version for actor code id
     *
     * @param actorCid - actor code id
     * @return v0, v2 or v3 actor version
     */
    static ActorVersion getActorVersionForCid(const CodeId &actorCid);

    static AddressMatcherPtr createAddressMatcher(ActorVersion version);
    static AddressMatcherPtr createAddressMatcher(
        const NetworkVersion &network_version);

    static InitUtilsPtr createInitActorUtils(Runtime &runtime);
    static MarketUtilsPtr createMarketUtils(Runtime &runtime);
    static MinerUtilsPtr createMinerUtils(Runtime &runtime);
    static MultisigUtilsPtr createMultisigActorUtils(Runtime &runtime);
    static PaymentChannelUtilsPtr createPaymentChannelUtils(Runtime &runtime);
    static PowerUtilsPtr createPowerUtils(Runtime &runtime);
    static VerifRegUtilsPtr createVerifRegUtils(Runtime &runtime);
  };
}  // namespace fc::vm::toolchain
