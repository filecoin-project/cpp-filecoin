/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/toolchain/impl/address_matcher_v4.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::toolchain {
  using namespace actor::builtin;

  bool AddressMatcherV4::isAccountActor(const CodeId &code) const {
    return code == v4::kAccountCodeId;
  }

  bool AddressMatcherV4::isCronActor(const CodeId &code) const {
    return code == v4::kCronCodeId;
  }

  bool AddressMatcherV4::isStoragePowerActor(const CodeId &code) const {
    return code == v4::kStoragePowerCodeId;
  }

  bool AddressMatcherV4::isStorageMarketActor(const CodeId &code) const {
    return code == v4::kStorageMarketCodeId;
  }

  bool AddressMatcherV4::isStorageMinerActor(const CodeId &code) const {
    return code == v4::kStorageMinerCodeId;
  }

  bool AddressMatcherV4::isMultisigActor(const CodeId &code) const {
    return code == v4::kMultisigCodeId;
  }

  bool AddressMatcherV4::isInitActor(const CodeId &code) const {
    return code == v4::kInitCodeId;
  }

  bool AddressMatcherV4::isPaymentChannelActor(const CodeId &code) const {
    return code == v4::kPaymentChannelCodeId;
  }

  bool AddressMatcherV4::isRewardActor(const CodeId &code) const {
    return code == v4::kRewardActorCodeId;
  }

  bool AddressMatcherV4::isSystemActor(const CodeId &code) const {
    return code == v4::kSystemActorCodeId;
  }

  bool AddressMatcherV4::isVerifiedRegistryActor(const CodeId &code) const {
    return code == v4::kVerifiedRegistryCodeId;
  }

  bool AddressMatcherV4::isBuiltinActor(const CodeId &code) const {
    return code == v4::kStorageMarketCodeId || code == v4::kStoragePowerCodeId
           || code == v4::kStorageMinerCodeId || code == v4::kAccountCodeId
           || code == v4::kInitCodeId || code == v4::kMultisigCodeId
           || code == v4::kPaymentChannelCodeId || code == v4::kCronCodeId
           || code == v4::kRewardActorCodeId || code == v4::kSystemActorCodeId;
  }

  bool AddressMatcherV4::isSingletonActor(const CodeId &code) const {
    return code == v4::kStoragePowerCodeId || code == v4::kStorageMarketCodeId
           || code == v4::kInitCodeId || code == v4::kCronCodeId
           || code == v4::kRewardActorCodeId || code == v4::kSystemActorCodeId;
  }

  bool AddressMatcherV4::isSignableActor(const CodeId &code) const {
    return code == v4::kAccountCodeId || code == v4::kMultisigCodeId;
  }

  CodeId AddressMatcherV4::getAccountCodeId() const {
    return CodeId{v4::kAccountCodeId};
  }

  CodeId AddressMatcherV4::getCronCodeId() const {
    return CodeId{v4::kCronCodeId};
  }

  CodeId AddressMatcherV4::getStoragePowerCodeId() const {
    return CodeId{v4::kStoragePowerCodeId};
  }

  CodeId AddressMatcherV4::getStorageMarketCodeId() const {
    return CodeId{v4::kStorageMarketCodeId};
  }

  CodeId AddressMatcherV4::getStorageMinerCodeId() const {
    return CodeId{v4::kStorageMinerCodeId};
  }

  CodeId AddressMatcherV4::getMultisigCodeId() const {
    return CodeId{v4::kMultisigCodeId};
  }

  CodeId AddressMatcherV4::getInitCodeId() const {
    return CodeId{v4::kInitCodeId};
  }

  CodeId AddressMatcherV4::getPaymentChannelCodeId() const {
    return CodeId{v4::kPaymentChannelCodeId};
  }

  CodeId AddressMatcherV4::getRewardActorCodeId() const {
    return CodeId{v4::kRewardActorCodeId};
  }

  CodeId AddressMatcherV4::getSystemActorCodeId() const {
    return CodeId{v4::kSystemActorCodeId};
  }

  CodeId AddressMatcherV4::getVerifiedRegistryCodeId() const {
    return CodeId{v4::kVerifiedRegistryCodeId};
  }

}  // namespace fc::vm::toolchain
