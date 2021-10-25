/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/codes.hpp"
#include "vm/toolchain/address_matcher.hpp"

#define ADDRESS_MATCHER_IMPL(name, codes)                                     \
  struct name : public AddressMatcher {                                       \
    bool isAccountActor(const CodeId &code) const override {                  \
      return code == codes::kAccountCodeId;                                   \
    }                                                                         \
    bool isCronActor(const CodeId &code) const override {                     \
      return code == codes::kCronCodeId;                                      \
    }                                                                         \
    bool isStoragePowerActor(const CodeId &code) const override {             \
      return code == codes::kStoragePowerCodeId;                              \
    }                                                                         \
    bool isStorageMarketActor(const CodeId &code) const override {            \
      return code == codes::kStorageMarketCodeId;                             \
    }                                                                         \
    bool isStorageMinerActor(const CodeId &code) const override {             \
      return code == codes::kStorageMinerCodeId;                              \
    }                                                                         \
    bool isMultisigActor(const CodeId &code) const override {                 \
      return code == codes::kMultisigCodeId;                                  \
    }                                                                         \
    bool isInitActor(const CodeId &code) const override {                     \
      return code == codes::kInitCodeId;                                      \
    }                                                                         \
    bool isPaymentChannelActor(const CodeId &code) const override {           \
      return code == codes::kPaymentChannelCodeId;                            \
    }                                                                         \
    bool isRewardActor(const CodeId &code) const override {                   \
      return code == codes::kRewardActorCodeId;                               \
    }                                                                         \
    bool isSystemActor(const CodeId &code) const override {                   \
      return code == codes::kSystemActorCodeId;                               \
    }                                                                         \
    bool isVerifiedRegistryActor(const CodeId &code) const override {         \
      return code == codes::kVerifiedRegistryCodeId;                          \
    }                                                                         \
    bool isBuiltinActor(const CodeId &code) const override {                  \
      return code == codes::kStorageMarketCodeId                              \
             || code == codes::kStoragePowerCodeId                            \
             || code == codes::kStorageMinerCodeId                            \
             || code == codes::kAccountCodeId || code == codes::kInitCodeId   \
             || code == codes::kMultisigCodeId                                \
             || code == codes::kPaymentChannelCodeId                          \
             || code == codes::kCronCodeId                                    \
             || code == codes::kRewardActorCodeId                             \
             || code == codes::kSystemActorCodeId;                            \
    }                                                                         \
    bool isSingletonActor(const CodeId &code) const override {                \
      return code == codes::kStoragePowerCodeId                               \
             || code == codes::kStorageMarketCodeId                           \
             || code == codes::kInitCodeId || code == codes::kCronCodeId      \
             || code == codes::kRewardActorCodeId                             \
             || code == codes::kSystemActorCodeId;                            \
    }                                                                         \
    bool isSignableActor(const CodeId &code) const override {                 \
      return code == codes::kAccountCodeId || code == codes::kMultisigCodeId; \
    }                                                                         \
    CodeId getAccountCodeId() const override {                                \
      return codes::kAccountCodeId;                                           \
    }                                                                         \
    CodeId getCronCodeId() const override {                                   \
      return codes::kCronCodeId;                                              \
    }                                                                         \
    CodeId getStoragePowerCodeId() const override {                           \
      return codes::kStoragePowerCodeId;                                      \
    }                                                                         \
    CodeId getStorageMarketCodeId() const override {                          \
      return codes::kStorageMarketCodeId;                                     \
    }                                                                         \
    CodeId getStorageMinerCodeId() const override {                           \
      return codes::kStorageMinerCodeId;                                      \
    }                                                                         \
    CodeId getMultisigCodeId() const override {                               \
      return codes::kMultisigCodeId;                                          \
    }                                                                         \
    CodeId getInitCodeId() const override {                                   \
      return codes::kInitCodeId;                                              \
    }                                                                         \
    CodeId getPaymentChannelCodeId() const override {                         \
      return codes::kPaymentChannelCodeId;                                    \
    }                                                                         \
    CodeId getRewardActorCodeId() const override {                            \
      return codes::kRewardActorCodeId;                                       \
    }                                                                         \
    CodeId getSystemActorCodeId() const override {                            \
      return codes::kSystemActorCodeId;                                       \
    }                                                                         \
    CodeId getVerifiedRegistryCodeId() const override {                       \
      return codes::kVerifiedRegistryCodeId;                                  \
    }                                                                         \
  };

namespace fc::vm::toolchain {
  ADDRESS_MATCHER_IMPL(AddressMatcherV0, actor::builtin::v0)
  ADDRESS_MATCHER_IMPL(AddressMatcherV2, actor::builtin::v2)
  ADDRESS_MATCHER_IMPL(AddressMatcherV3, actor::builtin::v3)
  ADDRESS_MATCHER_IMPL(AddressMatcherV4, actor::builtin::v4)
  ADDRESS_MATCHER_IMPL(AddressMatcherV5, actor::builtin::v5)
  ADDRESS_MATCHER_IMPL(AddressMatcherV6, actor::builtin::v6)
}  // namespace fc::vm::toolchain
