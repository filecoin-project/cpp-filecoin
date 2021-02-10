/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v3.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV3::isAccountActor(const CodeId &code) const {
    return code == v3::kAccountCodeId;
  }

  bool ActorContextV3::isStorageMinerActor(const CodeId &code) const {
    return code == v3::kStorageMinerCodeId;
  }

  bool ActorContextV3::isBuiltinActor(const CodeId &code) const {
    return code == v3::kStorageMarketCodeId || code == v3::kStoragePowerCodeId
           || code == v3::kStorageMinerCodeId || code == v3::kAccountCodeId
           || code == v3::kInitCodeId || code == v3::kMultisigCodeId
           || code == v3::kPaymentChannelCodeCid || code == v3::kCronCodeId
           || code == v3::kRewardActorCodeId || code == v3::kSystemActorCodeId;
  }

  bool ActorContextV3::isSingletonActor(const CodeId &code) const {
    return code == v3::kStoragePowerCodeId || code == v3::kStorageMarketCodeId
           || code == v3::kInitCodeId || code == v3::kCronCodeId
           || code == v3::kRewardActorCodeId || code == v3::kSystemActorCodeId;
  }

  bool ActorContextV3::isSignableActor(const CodeId &code) const {
    return code == v3::kAccountCodeId || code == v3::kMultisigCodeId;
  }

  CodeId ActorContextV3::getAccountCodeId() const {
    return v3::kAccountCodeId;
  }

  CodeId ActorContextV3::getCronCodeId() const {
    return v3::kCronCodeId;
  }

  CodeId ActorContextV3::getStoragePowerCodeId() const {
    return v3::kStoragePowerCodeId;
  }

  CodeId ActorContextV3::getStorageMarketCodeId() const {
    return v3::kStorageMarketCodeId;
  }

  CodeId ActorContextV3::getStorageMinerCodeId() const {
    return v3::kStorageMinerCodeId;
  }

  CodeId ActorContextV3::getMultisigCodeId() const {
    return v3::kMultisigCodeId;
  }

  CodeId ActorContextV3::getInitCodeId() const {
    return v3::kInitCodeId;
  }

  CodeId ActorContextV3::getPaymentChannelCodeId() const {
    return v3::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV3::getRewardActorCodeId() const {
    return v3::kRewardActorCodeId;
  }

  CodeId ActorContextV3::getSystemActorCodeId() const {
    return v3::kSystemActorCodeId;
  }

  CodeId ActorContextV3::getVerifiedRegistryCodeId() const {
    return v3::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::runtime::context
