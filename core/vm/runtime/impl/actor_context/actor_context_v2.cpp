/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/impl/actor_context/actor_context_v2.hpp"
#include "vm/actor/builtin/v2/codes.hpp"

namespace fc::vm::runtime::context {
  using namespace actor::builtin;

  bool ActorContextV2::isAccountActor(const CodeId &code) const {
    return code == v2::kAccountCodeId;
  }

  bool ActorContextV2::isStorageMinerActor(const CodeId &code) const {
    return code == v2::kStorageMinerCodeId;
  }

  bool ActorContextV2::isBuiltinActor(const CodeId &code) const {
    return code == v2::kStorageMarketCodeId || code == v2::kStoragePowerCodeId
           || code == v2::kStorageMinerCodeId || code == v2::kAccountCodeId
           || code == v2::kInitCodeId || code == v2::kMultisigCodeId
           || code == v2::kPaymentChannelCodeCid || code == v2::kCronCodeId
           || code == v2::kRewardActorCodeId || code == v2::kSystemActorCodeId;
  }

  bool ActorContextV2::isSingletonActor(const CodeId &code) const {
    return code == v2::kStoragePowerCodeId || code == v2::kStorageMarketCodeId
           || code == v2::kInitCodeId || code == v2::kCronCodeId
           || code == v2::kRewardActorCodeId || code == v2::kSystemActorCodeId;
  }

  bool ActorContextV2::isSignableActor(const CodeId &code) const {
    return code == v2::kAccountCodeId || code == v2::kMultisigCodeId;
  }

  CodeId ActorContextV2::getAccountCodeId() const {
    return v2::kAccountCodeId;
  }

  CodeId ActorContextV2::getCronCodeId() const {
    return v2::kCronCodeId;
  }

  CodeId ActorContextV2::getStoragePowerCodeId() const {
    return v2::kStoragePowerCodeId;
  }

  CodeId ActorContextV2::getStorageMarketCodeId() const {
    return v2::kStorageMarketCodeId;
  }

  CodeId ActorContextV2::getStorageMinerCodeId() const {
    return v2::kStorageMinerCodeId;
  }

  CodeId ActorContextV2::getMultisigCodeId() const {
    return v2::kMultisigCodeId;
  }

  CodeId ActorContextV2::getInitCodeId() const {
    return v2::kInitCodeId;
  }

  CodeId ActorContextV2::getPaymentChannelCodeId() const {
    return v2::kPaymentChannelCodeCid;
  }

  CodeId ActorContextV2::getRewardActorCodeId() const {
    return v2::kRewardActorCodeId;
  }

  CodeId ActorContextV2::getSystemActorCodeId() const {
    return v2::kSystemActorCodeId;
  }

  CodeId ActorContextV2::getVerifiedRegistryCodeId() const {
    return v2::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::runtime::context
