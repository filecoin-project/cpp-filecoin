/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/impl/address_matcher_v5.hpp"
#include "vm/actor/builtin/v5/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool AddressMatcherV5::isAccountActor(const CodeId &code) const {
    return code == v5::kAccountCodeId;
  }

  bool AddressMatcherV5::isCronActor(const CodeId &code) const {
    return code == v5::kCronCodeId;
  }

  bool AddressMatcherV5::isStoragePowerActor(const CodeId &code) const {
    return code == v5::kStoragePowerCodeId;
  }

  bool AddressMatcherV5::isStorageMarketActor(const CodeId &code) const {
    return code == v5::kStorageMarketCodeId;
  }

  bool AddressMatcherV5::isStorageMinerActor(const CodeId &code) const {
    return code == v5::kStorageMinerCodeId;
  }

  bool AddressMatcherV5::isMultisigActor(const CodeId &code) const {
    return code == v5::kMultisigCodeId;
  }

  bool AddressMatcherV5::isInitActor(const CodeId &code) const {
    return code == v5::kInitCodeId;
  }

  bool AddressMatcherV5::isPaymentChannelActor(const CodeId &code) const {
    return code == v5::kPaymentChannelCodeId;
  }

  bool AddressMatcherV5::isRewardActor(const CodeId &code) const {
    return code == v5::kRewardActorCodeId;
  }

  bool AddressMatcherV5::isSystemActor(const CodeId &code) const {
    return code == v5::kSystemActorCodeId;
  }

  bool AddressMatcherV5::isVerifiedRegistryActor(const CodeId &code) const {
    return code == v5::kVerifiedRegistryCodeId;
  }

  bool AddressMatcherV5::isBuiltinActor(const CodeId &code) const {
    return code == v5::kStorageMarketCodeId || code == v5::kStoragePowerCodeId
           || code == v5::kStorageMinerCodeId || code == v5::kAccountCodeId
           || code == v5::kInitCodeId || code == v5::kMultisigCodeId
           || code == v5::kPaymentChannelCodeId || code == v5::kCronCodeId
           || code == v5::kRewardActorCodeId || code == v5::kSystemActorCodeId;
  }

  bool AddressMatcherV5::isSingletonActor(const CodeId &code) const {
    return code == v5::kStoragePowerCodeId || code == v5::kStorageMarketCodeId
           || code == v5::kInitCodeId || code == v5::kCronCodeId
           || code == v5::kRewardActorCodeId || code == v5::kSystemActorCodeId;
  }

  bool AddressMatcherV5::isSignableActor(const CodeId &code) const {
    return code == v5::kAccountCodeId || code == v5::kMultisigCodeId;
  }

  CodeId AddressMatcherV5::getAccountCodeId() const {
    return v5::kAccountCodeId;
  }

  CodeId AddressMatcherV5::getCronCodeId() const {
    return v5::kCronCodeId;
  }

  CodeId AddressMatcherV5::getStoragePowerCodeId() const {
    return v5::kStoragePowerCodeId;
  }

  CodeId AddressMatcherV5::getStorageMarketCodeId() const {
    return v5::kStorageMarketCodeId;
  }

  CodeId AddressMatcherV5::getStorageMinerCodeId() const {
    return v5::kStorageMinerCodeId;
  }

  CodeId AddressMatcherV5::getMultisigCodeId() const {
    return v5::kMultisigCodeId;
  }

  CodeId AddressMatcherV5::getInitCodeId() const {
    return v5::kInitCodeId;
  }

  CodeId AddressMatcherV5::getPaymentChannelCodeId() const {
    return v5::kPaymentChannelCodeId;
  }

  CodeId AddressMatcherV5::getRewardActorCodeId() const {
    return v5::kRewardActorCodeId;
  }

  CodeId AddressMatcherV5::getSystemActorCodeId() const {
    return v5::kSystemActorCodeId;
  }

  CodeId AddressMatcherV5::getVerifiedRegistryCodeId() const {
    return v5::kVerifiedRegistryCodeId;
  }

}  // namespace fc::vm::toolchain
