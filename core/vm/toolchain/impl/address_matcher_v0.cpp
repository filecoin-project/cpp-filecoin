/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/impl/address_matcher_v0.hpp"
#include "vm/actor/builtin/v0/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool AddressMatcherV0::isAccountActor(const CodeId &code) const {
    return code == v0::kAccountCodeId;
  }

  bool AddressMatcherV0::isStorageMinerActor(const CodeId &code) const {
    return code == v0::kStorageMinerCodeId;
  }

  bool AddressMatcherV0::isBuiltinActor(const CodeId &code) const {
    return code == v0::kStorageMarketCodeId || code == v0::kStoragePowerCodeId
           || code == v0::kStorageMinerCodeId || code == v0::kAccountCodeId
           || code == v0::kInitCodeId || code == v0::kMultisigCodeId
           || code == v0::kPaymentChannelCodeId || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId;
  }

  bool AddressMatcherV0::isSingletonActor(const CodeId &code) const {
    return code == v0::kStoragePowerCodeId || code == v0::kStorageMarketCodeId
           || code == v0::kInitCodeId || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId;
  }

  bool AddressMatcherV0::isSignableActor(const CodeId &code) const {
    return code == v0::kAccountCodeId || code == v0::kMultisigCodeId;
  }

  CodeId AddressMatcherV0::getAccountCodeId() const {
    return v0::kAccountCodeId;
  }

  CodeId AddressMatcherV0::getCronCodeId() const {
    return v0::kCronCodeId;
  }

  CodeId AddressMatcherV0::getStoragePowerCodeId() const {
    return v0::kStoragePowerCodeId;
  }

  CodeId AddressMatcherV0::getStorageMarketCodeId() const {
    return v0::kStorageMarketCodeId;
  }

  CodeId AddressMatcherV0::getStorageMinerCodeId() const {
    return v0::kStorageMinerCodeId;
  }

  CodeId AddressMatcherV0::getMultisigCodeId() const {
    return v0::kMultisigCodeId;
  }

  CodeId AddressMatcherV0::getInitCodeId() const {
    return v0::kInitCodeId;
  }

  CodeId AddressMatcherV0::getPaymentChannelCodeId() const {
    return v0::kPaymentChannelCodeId;
  }

  CodeId AddressMatcherV0::getRewardActorCodeId() const {
    return v0::kRewardActorCodeId;
  }

  CodeId AddressMatcherV0::getSystemActorCodeId() const {
    return v0::kSystemActorCodeId;
  }

  CodeId AddressMatcherV0::getVerifiedRegistryCodeId() const {
    return v0::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::toolchain
