/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v0.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV0::isAccountActor(const CodeId &code) const {
    return code == v0::kAccountCodeCid;
  }

  bool ActorContextV0::isStorageMinerActor(const CodeId &code) const {
    return code == v0::kStorageMinerCodeCid;
  }

  bool ActorContextV0::isBuiltinActor(const CodeId &code) const {
    return code == v0::kStorageMarketCodeCid || code == v0::kStoragePowerCodeCid
           || code == v0::kStorageMinerCodeCid || code == v0::kAccountCodeCid
           || code == v0::kInitCodeCid || code == v0::kMultisigCodeCid
           || code == v0::kPaymentChannelCodeCid || code == v0::kCronCodeCid
           || code == v0::kRewardActorCodeID || code == v0::kSystemActorCodeID;
  }

  bool ActorContextV0::isSingletonActor(const CodeId &code) const {
    return code == v0::kStoragePowerCodeCid || code == v0::kStorageMarketCodeCid
           || code == v0::kInitCodeCid || code == v0::kCronCodeCid
           || code == v0::kRewardActorCodeID || code == v0::kSystemActorCodeID;
  }

  bool ActorContextV0::isSignableActor(const CodeId &code) const {
    return code == v0::kAccountCodeCid || code == v0::kMultisigCodeCid;
  }

  CodeId ActorContextV0::getAccountCodeId() const {
    return v0::kAccountCodeCid;
  }

  CodeId ActorContextV0::getCronCodeId() const {
    return v0::kCronCodeCid;
  }

  CodeId ActorContextV0::getStoragePowerCodeId() const {
    return v0::kStoragePowerCodeCid;
  }

  CodeId ActorContextV0::getStorageMarketCodeId() const {
    return v0::kStorageMarketCodeCid;
  }

  CodeId ActorContextV0::getStorageMinerCodeId() const {
    return v0::kStorageMinerCodeCid;
  }

  CodeId ActorContextV0::getMultisigCodeId() const {
    return v0::kMultisigCodeCid;
  }

  CodeId ActorContextV0::getInitCodeId() const {
    return v0::kInitCodeCid;
  }

  CodeId ActorContextV0::getPaymentChannelCodeId() const {
    return v0::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV0::getRewardActorCodeId() const {
    return v0::kRewardActorCodeID;
  }

  CodeId ActorContextV0::getSystemActorCodeId() const {
    return v0::kSystemActorCodeID;
  }

  CodeId ActorContextV0::getVerifiedRegistryCodeId() const {
    return v0::kVerifiedRegistryCode;
  }

}  // namespace fc::vm::runtime::context
