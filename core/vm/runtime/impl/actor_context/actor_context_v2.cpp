/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v2.hpp"
#include "vm/actor/builtin/v2/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV2::isAccountActor(const CodeId &code) const {
    return code == v2::kAccountCodeCid;
  }

  bool ActorContextV2::isStorageMinerActor(const CodeId &code) const {
    return code == v2::kStorageMinerCodeCid;
  }

  bool ActorContextV2::isBuiltinActor(const CodeId &code) const {
    return code == v2::kStorageMarketCodeCid || code == v2::kStoragePowerCodeCid
           || code == v2::kStorageMinerCodeCid || code == v2::kAccountCodeCid
           || code == v2::kInitCodeCid || code == v2::kMultisigCodeCid
           || code == v2::kPaymentChannelCodeCid || code == v2::kCronCodeCid
           || code == v2::kRewardActorCodeID || code == v2::kSystemActorCodeID;
  }

  bool ActorContextV2::isSingletonActor(const CodeId &code) const {
    return code == v2::kStoragePowerCodeCid || code == v2::kStorageMarketCodeCid
           || code == v2::kInitCodeCid || code == v2::kCronCodeCid
           || code == v2::kRewardActorCodeID || code == v2::kSystemActorCodeID;
  }

  bool ActorContextV2::isSignableActor(const CodeId &code) const {
    return code == v2::kAccountCodeCid || code == v2::kMultisigCodeCid;
  }

  CodeId ActorContextV2::getAccountCodeId() const {
    return v2::kAccountCodeCid;
  }

  CodeId ActorContextV2::getCronCodeId() const {
    return v2::kCronCodeCid;
  }

  CodeId ActorContextV2::getStoragePowerCodeId() const {
    return v2::kStoragePowerCodeCid;
  }

  CodeId ActorContextV2::getStorageMarketCodeId() const {
    return v2::kStorageMarketCodeCid;
  }

  CodeId ActorContextV2::getStorageMinerCodeId() const {
    return v2::kStorageMinerCodeCid;
  }

  CodeId ActorContextV2::getMultisigCodeId() const {
    return v2::kMultisigCodeCid;
  }

  CodeId ActorContextV2::getInitCodeId() const {
    return v2::kInitCodeCid;
  }

  CodeId ActorContextV2::getPaymentChannelCodeId() const {
    return v2::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV2::getRewardActorCodeId() const {
    return v2::kRewardActorCodeID;
  }

  CodeId ActorContextV2::getSystemActorCodeId() const {
    return v2::kSystemActorCodeID;
  }

  CodeId ActorContextV2::getVerifiedRegistryCodeId() const {
    return v2::kVerifiedRegistryCode;
  }

}  // namespace fc::vm::runtime::context
