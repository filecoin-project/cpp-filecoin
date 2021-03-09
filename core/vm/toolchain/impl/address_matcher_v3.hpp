/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/toolchain/address_matcher.hpp"

namespace fc::vm::toolchain {
  class AddressMatcherV3 : public AddressMatcher {
   public:
    AddressMatcherV3() = default;

    bool isAccountActor(const CodeId &code) const override;
    bool isStorageMinerActor(const CodeId &code) const override;
    bool isPaymentChannelActor(const CodeId &code) const override;
    bool isBuiltinActor(const CodeId &code) const override;
    bool isSingletonActor(const CodeId &code) const override;
    bool isSignableActor(const CodeId &code) const override;

    CodeId getAccountCodeId() const override;
    CodeId getCronCodeId() const override;
    CodeId getStoragePowerCodeId() const override;
    CodeId getStorageMarketCodeId() const override;
    CodeId getStorageMinerCodeId() const override;
    CodeId getMultisigCodeId() const override;
    CodeId getInitCodeId() const override;
    CodeId getPaymentChannelCodeId() const override;
    CodeId getRewardActorCodeId() const override;
    CodeId getSystemActorCodeId() const override;
    CodeId getVerifiedRegistryCodeId() const override;
  };
}  // namespace fc::vm::toolchain
