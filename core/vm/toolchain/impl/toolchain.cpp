/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/toolchain.hpp"

#include "vm/actor/codes.hpp"

#include "vm/toolchain/impl/address_matcher.hpp"

#include "vm/actor/builtin/v0/market/market_actor_utils.hpp"
#include "vm/actor/builtin/v2/market/market_actor_utils.hpp"
#include "vm/actor/builtin/v3/market/market_actor_utils.hpp"

#include "vm/actor/builtin/v0/miner/miner_actor_utils.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_utils.hpp"
#include "vm/actor/builtin/v3/miner/miner_actor_utils.hpp"

#include "vm/actor/builtin/v0/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v2/multisig/multisig_actor_utils.hpp"
#include "vm/actor/builtin/v3/multisig/multisig_actor_utils.hpp"

#include "vm/actor/builtin/v0/payment_channel/payment_channel_actor_utils.hpp"
#include "vm/actor/builtin/v2/payment_channel/payment_channel_actor_utils.hpp"
#include "vm/actor/builtin/v3/payment_channel/payment_channel_actor_utils.hpp"

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_utils.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_utils.hpp"
#include "vm/actor/builtin/v3/storage_power/storage_power_actor_utils.hpp"

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/v2/verified_registry/verified_registry_actor_utils.hpp"
#include "vm/actor/builtin/v3/verified_registry/verified_registry_actor_utils.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

#define ACTOR_VERSION_FOR_CID(V)                                        \
  if ((actorCid == actor::builtin::v##V::kAccountCodeId)                \
      || (actorCid == actor::builtin::v##V::kCronCodeId)                \
      || (actorCid == actor::builtin::v##V::kStoragePowerCodeId)        \
      || (actorCid == actor::builtin::v##V::kStorageMarketCodeId)       \
      || (actorCid == actor::builtin::v##V::kStorageMinerCodeId)        \
      || (actorCid == actor::builtin::v##V::kMultisigCodeId)            \
      || (actorCid == actor::builtin::v##V::kInitCodeId)                \
      || (actorCid == actor::builtin::v##V::kPaymentChannelCodeId)      \
      || (actorCid == actor::builtin::v##V::kRewardActorCodeId)         \
      || (actorCid == actor::builtin::v##V::kSystemActorCodeId)         \
      || (actorCid == actor::builtin::v##V::kVerifiedRegistryCodeId)) { \
    return ActorVersion::kVersion##V;                                   \
  }

  ActorVersion Toolchain::getActorVersionForCid(const CodeId &actorCid) {
    ACTOR_VERSION_FOR_CID(0)
    ACTOR_VERSION_FOR_CID(2)
    ACTOR_VERSION_FOR_CID(3)
    ACTOR_VERSION_FOR_CID(4)
    ACTOR_VERSION_FOR_CID(5)
    ACTOR_VERSION_FOR_CID(6)
    ACTOR_VERSION_FOR_CID(7)

    assert(false);
    abort();
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(ActorVersion version) {
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<AddressMatcherV0>();
      case ActorVersion::kVersion2:
        return std::make_shared<AddressMatcherV2>();
      case ActorVersion::kVersion3:
        return std::make_shared<AddressMatcherV3>();
      case ActorVersion::kVersion4:
        return std::make_shared<AddressMatcherV4>();
      case ActorVersion::kVersion5:
        return std::make_shared<AddressMatcherV5>();
      case ActorVersion::kVersion6:
        return std::make_shared<AddressMatcherV6>();
      case ActorVersion::kVersion7:
        return std::make_shared<AddressMatcherV7>();
    }
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(
      const NetworkVersion &network_version) {
    const auto version = actorVersion(network_version);
    return createAddressMatcher(version);
  }

  InitUtilsPtr Toolchain::createInitActorUtils(Runtime &runtime) {
    return std::make_shared<actor::builtin::utils::InitUtils>(runtime);
  }

  MarketUtilsPtr Toolchain::createMarketUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::market::MarketUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::market::MarketUtils>(runtime);
      default:
        return std::make_shared<v3::market::MarketUtils>(runtime);
    }
  }

  MinerUtilsPtr Toolchain::createMinerUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::miner::MinerUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::miner::MinerUtils>(runtime);
      default:
        return std::make_shared<v3::miner::MinerUtils>(runtime);
    }
  }

  MultisigUtilsPtr Toolchain::createMultisigActorUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::multisig::MultisigUtils>(runtime);
      default:
        return std::make_shared<v3::multisig::MultisigUtils>(runtime);
    }
  }

  PaymentChannelUtilsPtr Toolchain::createPaymentChannelUtils(
      Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::payment_channel::PaymentChannelUtils>(
            runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::payment_channel::PaymentChannelUtils>(
            runtime);
      default:
        return std::make_shared<v3::payment_channel::PaymentChannelUtils>(
            runtime);
    }
  }

  PowerUtilsPtr Toolchain::createPowerUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::storage_power::PowerUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::storage_power::PowerUtils>(runtime);
      default:
        return std::make_shared<v3::storage_power::PowerUtils>(runtime);
    }
  }

  RewardUtilsPtr Toolchain::createRewardUtils(Runtime &runtime) {
    return std::make_shared<utils::RewardUtils>(runtime);
  }

  VerifRegUtilsPtr Toolchain::createVerifRegUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::verified_registry::VerifRegUtils>(runtime);
      default:
        return std::make_shared<v3::verified_registry::VerifRegUtils>(runtime);
    }
  }
}  // namespace fc::vm::toolchain
