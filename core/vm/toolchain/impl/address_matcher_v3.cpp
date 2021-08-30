/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/impl/address_matcher_v3.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool AddressMatcherV3::isAccountActor(const CodeId &code) const {
    return code == v3::kAccountCodeId;
  }

  bool AddressMatcherV3::isCronActor(const CodeId &code) const {
    return code == v3::kCronCodeId;
  }

  bool AddressMatcherV3::isStoragePowerActor(const CodeId &code) const {
    return code == v3::kStoragePowerCodeId;
  }

  bool AddressMatcherV3::isStorageMarketActor(const CodeId &code) const {
    return code == v3::kStorageMarketCodeId;
  }

  bool AddressMatcherV3::isStorageMinerActor(const CodeId &code) const {
    return code == v3::kStorageMinerCodeId;
  }

  bool AddressMatcherV3::isMultisigActor(const CodeId &code) const {
    return code == v3::kMultisigCodeId;
  }

  bool AddressMatcherV3::isInitActor(const CodeId &code) const {
    return code == v3::kInitCodeId;
  }

  bool AddressMatcherV3::isPaymentChannelActor(const CodeId &code) const {
    return code == v3::kPaymentChannelCodeId;
  }

  bool AddressMatcherV3::isRewardActor(const CodeId &code) const {
    return code == v3::kRewardActorCodeId;
  }

  bool AddressMatcherV3::isSystemActor(const CodeId &code) const {
    return code == v3::kSystemActorCodeId;
  }

  bool AddressMatcherV3::isVerifiedRegistryActor(const CodeId &code) const {
    return code == v3::kVerifiedRegistryCodeId;
  }

  bool AddressMatcherV3::isBuiltinActor(const CodeId &code) const {
    return code == v3::kStorageMarketCodeId || code == v3::kStoragePowerCodeId
           || code == v3::kStorageMinerCodeId || code == v3::kAccountCodeId
           || code == v3::kInitCodeId || code == v3::kMultisigCodeId
           || code == v3::kPaymentChannelCodeId || code == v3::kCronCodeId
           || code == v3::kRewardActorCodeId || code == v3::kSystemActorCodeId;
  }

  bool AddressMatcherV3::isSingletonActor(const CodeId &code) const {
    return code == v3::kStoragePowerCodeId || code == v3::kStorageMarketCodeId
           || code == v3::kInitCodeId || code == v3::kCronCodeId
           || code == v3::kRewardActorCodeId || code == v3::kSystemActorCodeId;
  }

  bool AddressMatcherV3::isSignableActor(const CodeId &code) const {
    return code == v3::kAccountCodeId || code == v3::kMultisigCodeId;
  }

  CodeId AddressMatcherV3::getAccountCodeId() const {
    return CodeId{v3::kAccountCodeId};
  }

  CodeId AddressMatcherV3::getCronCodeId() const {
    return CodeId{v3::kCronCodeId};
  }

  CodeId AddressMatcherV3::getStoragePowerCodeId() const {
    return CodeId{v3::kStoragePowerCodeId};
  }

  CodeId AddressMatcherV3::getStorageMarketCodeId() const {
    return CodeId{v3::kStorageMarketCodeId};
  }

  CodeId AddressMatcherV3::getStorageMinerCodeId() const {
    return CodeId{v3::kStorageMinerCodeId};
  }

  CodeId AddressMatcherV3::getMultisigCodeId() const {
    return CodeId{v3::kMultisigCodeId};
  }

  CodeId AddressMatcherV3::getInitCodeId() const {
    return CodeId{v3::kInitCodeId};
  }

  CodeId AddressMatcherV3::getPaymentChannelCodeId() const {
    return CodeId{v3::kPaymentChannelCodeId};
  }

  CodeId AddressMatcherV3::getRewardActorCodeId() const {
    return CodeId{v3::kRewardActorCodeId};
  }

  CodeId AddressMatcherV3::getSystemActorCodeId() const {
    return CodeId{v3::kSystemActorCodeId};
  }

  CodeId AddressMatcherV3::getVerifiedRegistryCodeId() const {
    return CodeId{v3::kVerifiedRegistryCodeId};
  }

}  // namespace fc::vm::toolchain
