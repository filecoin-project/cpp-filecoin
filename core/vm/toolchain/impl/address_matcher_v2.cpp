/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/impl/address_matcher_v2.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool AddressMatcherV2::isAccountActor(const CodeId &code) const {
    return code == v2::kAccountCodeId;
  }

  bool AddressMatcherV2::isCronActor(const CodeId &code) const {
    return code == v2::kCronCodeId;
  }

  bool AddressMatcherV2::isStoragePowerActor(const CodeId &code) const {
    return code == v2::kStoragePowerCodeId;
  }

  bool AddressMatcherV2::isStorageMarketActor(const CodeId &code) const {
    return code == v2::kStorageMarketCodeId;
  }

  bool AddressMatcherV2::isStorageMinerActor(const CodeId &code) const {
    return code == v2::kStorageMinerCodeId;
  }

  bool AddressMatcherV2::isMultisigActor(const CodeId &code) const {
    return code == v2::kMultisigCodeId;
  }

  bool AddressMatcherV2::isInitActor(const CodeId &code) const {
    return code == v2::kInitCodeId;
  }

  bool AddressMatcherV2::isPaymentChannelActor(const CodeId &code) const {
    return code == v2::kPaymentChannelCodeId;
  }

  bool AddressMatcherV2::isRewardActor(const CodeId &code) const {
    return code == v2::kRewardActorCodeId;
  }

  bool AddressMatcherV2::isSystemActor(const CodeId &code) const {
    return code == v2::kSystemActorCodeId;
  }

  bool AddressMatcherV2::isVerifiedRegistryActor(const CodeId &code) const {
    return code == v2::kVerifiedRegistryCodeId;
  }

  bool AddressMatcherV2::isBuiltinActor(const CodeId &code) const {
    return code == v2::kStorageMarketCodeId || code == v2::kStoragePowerCodeId
           || code == v2::kStorageMinerCodeId || code == v2::kAccountCodeId
           || code == v2::kInitCodeId || code == v2::kMultisigCodeId
           || code == v2::kPaymentChannelCodeId || code == v2::kCronCodeId
           || code == v2::kRewardActorCodeId || code == v2::kSystemActorCodeId;
  }

  bool AddressMatcherV2::isSingletonActor(const CodeId &code) const {
    return code == v2::kStoragePowerCodeId || code == v2::kStorageMarketCodeId
           || code == v2::kInitCodeId || code == v2::kCronCodeId
           || code == v2::kRewardActorCodeId || code == v2::kSystemActorCodeId;
  }

  bool AddressMatcherV2::isSignableActor(const CodeId &code) const {
    return code == v2::kAccountCodeId || code == v2::kMultisigCodeId;
  }

  CodeId AddressMatcherV2::getAccountCodeId() const {
    return v2::kAccountCodeId;
  }

  CodeId AddressMatcherV2::getCronCodeId() const {
    return v2::kCronCodeId;
  }

  CodeId AddressMatcherV2::getStoragePowerCodeId() const {
    return v2::kStoragePowerCodeId;
  }

  CodeId AddressMatcherV2::getStorageMarketCodeId() const {
    return v2::kStorageMarketCodeId;
  }

  CodeId AddressMatcherV2::getStorageMinerCodeId() const {
    return v2::kStorageMinerCodeId;
  }

  CodeId AddressMatcherV2::getMultisigCodeId() const {
    return v2::kMultisigCodeId;
  }

  CodeId AddressMatcherV2::getInitCodeId() const {
    return v2::kInitCodeId;
  }

  CodeId AddressMatcherV2::getPaymentChannelCodeId() const {
    return v2::kPaymentChannelCodeId;
  }

  CodeId AddressMatcherV2::getRewardActorCodeId() const {
    return v2::kRewardActorCodeId;
  }

  CodeId AddressMatcherV2::getSystemActorCodeId() const {
    return v2::kSystemActorCodeId;
  }

  CodeId AddressMatcherV2::getVerifiedRegistryCodeId() const {
    return v2::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::toolchain