/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/common_address_matcher.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool CommonAddressMatcher::isAccountActor(const CodeId &code) {
    return code == v0::kAccountCodeId || code == v2::kAccountCodeId
           || code == v3::kAccountCodeId;
  }

  bool CommonAddressMatcher::isCronActor(const CodeId &code) {
    return code == v0::kCronCodeId || code == v2::kCronCodeId
           || code == v3::kCronCodeId;
  }

  bool CommonAddressMatcher::isStoragePowerActor(const CodeId &code) {
    return code == v0::kStoragePowerCodeId || code == v2::kStoragePowerCodeId
           || code == v3::kStoragePowerCodeId;
  }

  bool CommonAddressMatcher::isStorageMarketActor(const CodeId &code) {
    return code == v0::kStorageMarketCodeId || code == v2::kStorageMarketCodeId
           || code == v3::kStorageMarketCodeId;
  }

  bool CommonAddressMatcher::isStorageMinerActor(const CodeId &code) {
    return code == v0::kStorageMinerCodeId || code == v2::kStorageMinerCodeId
           || code == v3::kStorageMinerCodeId;
  }

  bool CommonAddressMatcher::isMultisigActor(const CodeId &code) {
    return code == v0::kMultisigCodeId || code == v2::kMultisigCodeId
           || code == v3::kMultisigCodeId;
  }

  bool CommonAddressMatcher::isInitActor(const CodeId &code) {
    return code == v0::kInitCodeId || code == v2::kInitCodeId
           || code == v3::kInitCodeId;
  }

  bool CommonAddressMatcher::isPaymentChannelActor(const CodeId &code) {
    return code == v0::kPaymentChannelCodeId
           || code == v2::kPaymentChannelCodeId
           || code == v3::kPaymentChannelCodeId;
  }

  bool CommonAddressMatcher::isRewardActor(const CodeId &code) {
    return code == v0::kRewardActorCodeId || code == v2::kRewardActorCodeId
           || code == v3::kRewardActorCodeId;
  }

  bool CommonAddressMatcher::isSystemActor(const CodeId &code) {
    return code == v0::kSystemActorCodeId || code == v2::kSystemActorCodeId
           || code == v3::kSystemActorCodeId;
  }

  bool CommonAddressMatcher::isVerifiedRegistryActor(const CodeId &code) {
    return code == v0::kVerifiedRegistryCodeId
           || code == v2::kVerifiedRegistryCodeId
           || code == v3::kVerifiedRegistryCodeId;
  }
}  // namespace fc::vm::toolchain
