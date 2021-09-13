/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/toolchain.hpp"

#include "vm/actor/codes.hpp"

#include "vm/toolchain/impl/address_matcher_v0.hpp"
#include "vm/toolchain/impl/address_matcher_v2.hpp"
#include "vm/toolchain/impl/address_matcher_v3.hpp"
#include "vm/toolchain/impl/address_matcher_v4.hpp"
#include "vm/toolchain/impl/address_matcher_v5.hpp"

#include "vm/actor/builtin/v0/init/init_actor_utils.hpp"
#include "vm/actor/builtin/v2/init/init_actor_utils.hpp"
#include "vm/actor/builtin/v3/init/init_actor_utils.hpp"

#include "vm/actor/builtin/v0/market/market_actor_utils.hpp"
#include "vm/actor/builtin/v2/market/market_actor_utils.hpp"

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

#include "vm/actor/builtin/v4/todo.hpp"

namespace fc::vm::toolchain {
  using namespace fc::vm::actor::builtin;

  ActorVersion Toolchain::getActorVersionForNetwork(
      const NetworkVersion &network_version) {
    return actorVersion(network_version);
  }

  ActorVersion Toolchain::getActorVersionForCid(const CodeId &actorCid) {
    if ((actorCid == v0::kAccountCodeId) || (actorCid == v0::kCronCodeId)
        || (actorCid == v0::kStoragePowerCodeId)
        || (actorCid == v0::kStorageMarketCodeId)
        || (actorCid == v0::kStorageMinerCodeId)
        || (actorCid == v0::kMultisigCodeId) || (actorCid == v0::kInitCodeId)
        || (actorCid == v0::kPaymentChannelCodeId)
        || (actorCid == v0::kRewardActorCodeId)
        || (actorCid == v0::kSystemActorCodeId)
        || (actorCid == v0::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion0;
    }

    if ((actorCid == v2::kAccountCodeId) || (actorCid == v2::kCronCodeId)
        || (actorCid == v2::kStoragePowerCodeId)
        || (actorCid == v2::kStorageMarketCodeId)
        || (actorCid == v2::kStorageMinerCodeId)
        || (actorCid == v2::kMultisigCodeId) || (actorCid == v2::kInitCodeId)
        || (actorCid == v2::kPaymentChannelCodeId)
        || (actorCid == v2::kRewardActorCodeId)
        || (actorCid == v2::kSystemActorCodeId)
        || (actorCid == v2::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion2;
    }

    if ((actorCid == v3::kAccountCodeId) || (actorCid == v3::kCronCodeId)
        || (actorCid == v3::kStoragePowerCodeId)
        || (actorCid == v3::kStorageMarketCodeId)
        || (actorCid == v3::kStorageMinerCodeId)
        || (actorCid == v3::kMultisigCodeId) || (actorCid == v3::kInitCodeId)
        || (actorCid == v3::kPaymentChannelCodeId)
        || (actorCid == v3::kRewardActorCodeId)
        || (actorCid == v3::kSystemActorCodeId)
        || (actorCid == v3::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion3;
    }

    if ((actorCid == v4::kAccountCodeId) || (actorCid == v4::kCronCodeId)
        || (actorCid == v4::kStoragePowerCodeId)
        || (actorCid == v4::kStorageMarketCodeId)
        || (actorCid == v4::kStorageMinerCodeId)
        || (actorCid == v4::kMultisigCodeId) || (actorCid == v4::kInitCodeId)
        || (actorCid == v4::kPaymentChannelCodeId)
        || (actorCid == v4::kRewardActorCodeId)
        || (actorCid == v4::kSystemActorCodeId)
        || (actorCid == v4::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion4;
    }

    if ((actorCid == v5::kAccountCodeId) || (actorCid == v5::kCronCodeId)
        || (actorCid == v5::kStoragePowerCodeId)
        || (actorCid == v5::kStorageMarketCodeId)
        || (actorCid == v5::kStorageMinerCodeId)
        || (actorCid == v5::kMultisigCodeId) || (actorCid == v5::kInitCodeId)
        || (actorCid == v5::kPaymentChannelCodeId)
        || (actorCid == v5::kRewardActorCodeId)
        || (actorCid == v5::kSystemActorCodeId)
        || (actorCid == v5::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion5;
    }

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
    }
  }

  AddressMatcherPtr Toolchain::createAddressMatcher(
      const NetworkVersion &network_version) {
    const auto version = getActorVersionForNetwork(network_version);
    return createAddressMatcher(version);
  }

  InitUtilsPtr Toolchain::createInitActorUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::init::InitUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::init::InitUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::init::InitUtils>(runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  MarketUtilsPtr Toolchain::createMarketUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::market::MarketUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::market::MarketUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v2::market::MarketUtils>(runtime);  // TODO v3
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  MinerUtilsPtr Toolchain::createMinerUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::miner::MinerUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::miner::MinerUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::miner::MinerUtils>(runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  MultisigUtilsPtr Toolchain::createMultisigActorUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::multisig::MultisigUtils>(runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
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
      case ActorVersion::kVersion3:
        return std::make_shared<v3::payment_channel::PaymentChannelUtils>(
            runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  PowerUtilsPtr Toolchain::createPowerUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::storage_power::PowerUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::storage_power::PowerUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::storage_power::PowerUtils>(runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  VerifRegUtilsPtr Toolchain::createVerifRegUtils(Runtime &runtime) {
    const auto version = runtime.getActorVersion();
    switch (version) {
      case ActorVersion::kVersion0:
        return std::make_shared<v0::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion2:
        return std::make_shared<v2::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion3:
        return std::make_shared<v3::verified_registry::VerifRegUtils>(runtime);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }
}  // namespace fc::vm::toolchain
