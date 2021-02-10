/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v0.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV0::isAccountActor(const CodeId &code) const {
    return code == v0::kAccountCodeId;
  }

  bool ActorContextV0::isStorageMinerActor(const CodeId &code) const {
    return code == v0::kStorageMinerCodeId;
  }

  bool ActorContextV0::isBuiltinActor(const CodeId &code) const {
    return code == v0::kStorageMarketCodeId || code == v0::kStoragePowerCodeId
           || code == v0::kStorageMinerCodeId || code == v0::kAccountCodeId
           || code == v0::kInitCodeId || code == v0::kMultisigCodeId
           || code == v0::kPaymentChannelCodeCid || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId;
  }

  bool ActorContextV0::isSingletonActor(const CodeId &code) const {
    return code == v0::kStoragePowerCodeId || code == v0::kStorageMarketCodeId
           || code == v0::kInitCodeId || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId;
  }

  bool ActorContextV0::isSignableActor(const CodeId &code) const {
    return code == v0::kAccountCodeId || code == v0::kMultisigCodeId;
  }

  CodeId ActorContextV0::getAccountCodeId() const {
    return v0::kAccountCodeId;
  }

  CodeId ActorContextV0::getCronCodeId() const {
    return v0::kCronCodeId;
  }

  CodeId ActorContextV0::getStoragePowerCodeId() const {
    return v0::kStoragePowerCodeId;
  }

  CodeId ActorContextV0::getStorageMarketCodeId() const {
    return v0::kStorageMarketCodeId;
  }

  CodeId ActorContextV0::getStorageMinerCodeId() const {
    return v0::kStorageMinerCodeId;
  }

  CodeId ActorContextV0::getMultisigCodeId() const {
    return v0::kMultisigCodeId;
  }

  CodeId ActorContextV0::getInitCodeId() const {
    return v0::kInitCodeId;
  }

  CodeId ActorContextV0::getPaymentChannelCodeId() const {
    return v0::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV0::getRewardActorCodeId() const {
    return v0::kRewardActorCodeId;
  }

  CodeId ActorContextV0::getSystemActorCodeId() const {
    return v0::kSystemActorCodeId;
  }

  CodeId ActorContextV0::getVerifiedRegistryCodeId() const {
    return v0::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::runtime::context
