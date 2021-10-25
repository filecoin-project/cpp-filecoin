/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/common_address_matcher.hpp"
#include "vm/actor/codes.hpp"

#define ACTOR_CODE_IS(name)                                                    \
  code == v0::name || code == v2::name || code == v3::name || code == v4::name \
      || code == v5::name || code == v6::name

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool CommonAddressMatcher::isAccountActor(const CodeId &code) {
    return ACTOR_CODE_IS(kAccountCodeId);
  }

  bool CommonAddressMatcher::isCronActor(const CodeId &code) {
    return ACTOR_CODE_IS(kCronCodeId);
  }

  bool CommonAddressMatcher::isStoragePowerActor(const CodeId &code) {
    return ACTOR_CODE_IS(kStoragePowerCodeId);
  }

  bool CommonAddressMatcher::isStorageMarketActor(const CodeId &code) {
    return ACTOR_CODE_IS(kStorageMarketCodeId);
  }

  bool CommonAddressMatcher::isStorageMinerActor(const CodeId &code) {
    return ACTOR_CODE_IS(kStorageMinerCodeId);
  }

  bool CommonAddressMatcher::isMultisigActor(const CodeId &code) {
    return ACTOR_CODE_IS(kMultisigCodeId);
  }

  bool CommonAddressMatcher::isInitActor(const CodeId &code) {
    return ACTOR_CODE_IS(kInitCodeId);
  }

  bool CommonAddressMatcher::isPaymentChannelActor(const CodeId &code) {
    return ACTOR_CODE_IS(kPaymentChannelCodeId);
  }

  bool CommonAddressMatcher::isRewardActor(const CodeId &code) {
    return ACTOR_CODE_IS(kRewardActorCodeId);
  }

  bool CommonAddressMatcher::isSystemActor(const CodeId &code) {
    return ACTOR_CODE_IS(kSystemActorCodeId);
  }

  bool CommonAddressMatcher::isVerifiedRegistryActor(const CodeId &code) {
    return ACTOR_CODE_IS(kVerifiedRegistryCodeId);
  }
}  // namespace fc::vm::toolchain
