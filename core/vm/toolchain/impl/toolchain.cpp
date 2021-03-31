/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/toolchain.hpp"

#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

#include "vm/toolchain/impl/address_matcher_v0.hpp"
#include "vm/toolchain/impl/address_matcher_v2.hpp"
#include "vm/toolchain/impl/address_matcher_v3.hpp"

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

namespace fc::vm::toolchain {
  using namespace fc::vm::actor::builtin;

  ActorVersion Toolchain::getActorVersionForNetwork(
      const NetworkVersion &network_version) {
    switch (network_version) {
      case NetworkVersion::kVersion0:
      case NetworkVersion::kVersion1:
      case NetworkVersion::kVersion2:
      case NetworkVersion::kVersion3:
        return ActorVersion::kVersion0;
      case NetworkVersion::kVersion4:
      case NetworkVersion::kVersion5:
      case NetworkVersion::kVersion6:
      case NetworkVersion::kVersion7:
      case NetworkVersion::kVersion8:
      case NetworkVersion::kVersion9:
        return ActorVersion::kVersion2;
      case NetworkVersion::kVersion10:
      case NetworkVersion::kVersion11:
      case NetworkVersion::kVersion12:
        return ActorVersion::kVersion3;
    }
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
    }
  }
}  // namespace fc::vm::toolchain
