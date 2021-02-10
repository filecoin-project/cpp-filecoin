/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v3.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV3::isAccountActor(const CodeId &code) const {
    return code == v3::kAccountCodeCid;
  }

  bool ActorContextV3::isStorageMinerActor(const CodeId &code) const {
    return code == v3::kStorageMinerCodeCid;
  }

  bool ActorContextV3::isBuiltinActor(const CodeId &code) const {
    return code == v3::kStorageMarketCodeCid || code == v3::kStoragePowerCodeCid
           || code == v3::kStorageMinerCodeCid || code == v3::kAccountCodeCid
           || code == v3::kInitCodeCid || code == v3::kMultisigCodeCid
           || code == v3::kPaymentChannelCodeCid || code == v3::kCronCodeCid
           || code == v3::kRewardActorCodeID || code == v3::kSystemActorCodeID;
  }

  bool ActorContextV3::isSingletonActor(const CodeId &code) const {
    return code == v3::kStoragePowerCodeCid || code == v3::kStorageMarketCodeCid
           || code == v3::kInitCodeCid || code == v3::kCronCodeCid
           || code == v3::kRewardActorCodeID || code == v3::kSystemActorCodeID;
  }

  bool ActorContextV3::isSignableActor(const CodeId &code) const {
    return code == v3::kAccountCodeCid || code == v3::kMultisigCodeCid;
  }

  CodeId ActorContextV3::getAccountCodeCid() const {
    return v3::kAccountCodeCid;
  }

  CodeId ActorContextV3::getCronCodeCid() const {
    return v3::kCronCodeCid;
  }

  CodeId ActorContextV3::getStoragePowerCodeCid() const {
    return v3::kStoragePowerCodeCid;
  }

  CodeId ActorContextV3::getStorageMarketCodeCid() const {
    return v3::kStorageMarketCodeCid;
  }

  CodeId ActorContextV3::getStorageMinerCodeCid() const {
    return v3::kStorageMinerCodeCid;
  }

  CodeId ActorContextV3::getMultisigCodeCid() const {
    return v3::kMultisigCodeCid;
  }

  CodeId ActorContextV3::getInitCodeCid() const {
    return v3::kInitCodeCid;
  }

  CodeId ActorContextV3::getPaymentChannelCodeCid() const {
    return v3::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV3::getRewardActorCodeID() const {
    return v3::kRewardActorCodeID;
  }

  CodeId ActorContextV3::getSystemActorCodeID() const {
    return v3::kSystemActorCodeID;
  }

  CodeId ActorContextV3::getVerifiedRegistryCodeId() const {
    return v3::kVerifiedRegistryCode;
  }

}  // namespace fc::vm::runtime::context
