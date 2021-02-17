/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#pragma once

namespace fc::vm::toolchain {
  using actor::CodeId;

  class AddressMatcher {
   public:
    virtual ~AddressMatcher() = default;

    /** Checks if code is an account actor */
    virtual bool isAccountActor(const CodeId &code) const = 0;

    /** Checks if code is cron actor */
    virtual bool isCronActor(const CodeId &code) const = 0;

    /** Checks if code is power actor */
    virtual bool isStoragePowerActor(const CodeId &code) const = 0;

    /** Checks if code is market actor */
    virtual bool isStorageMarketActor(const CodeId &code) const = 0;

    /** Checks if code is miner actor */
    virtual bool isStorageMinerActor(const CodeId &code) const = 0;

    /** Checks if code is multisig actor */
    virtual bool isMultisigActor(const CodeId &code) const = 0;

    /** Checks if code is init actor */
    virtual bool isInitActor(const CodeId &code) const = 0;

    /** Checks if code is payment channel actor */
    virtual bool isPaymentChannelActor(const CodeId &code) const = 0;

    /** Checks if code is reward actor */
    virtual bool isRewardActor(const CodeId &code) const = 0;

    /** Checks if code is system actor */
    virtual bool isSystemActor(const CodeId &code) const = 0;

    /** Checks if code is verified registry actor */
    virtual bool isVerifiedRegistryActor(const CodeId &code) const = 0;

    /** Check if code specifies builtin actor implementation */
    virtual bool isBuiltinActor(const CodeId &code) const = 0;

    /** Check if only one instance of actor should exists */
    virtual bool isSingletonActor(const CodeId &code) const = 0;

    /** Check if actor code can represent external signing parties */
    virtual bool isSignableActor(const CodeId &code) const = 0;

    /** Get actor's code id */
    virtual CodeId getAccountCodeId() const = 0;
    virtual CodeId getCronCodeId() const = 0;
    virtual CodeId getStoragePowerCodeId() const = 0;
    virtual CodeId getStorageMarketCodeId() const = 0;
    virtual CodeId getStorageMinerCodeId() const = 0;
    virtual CodeId getMultisigCodeId() const = 0;
    virtual CodeId getInitCodeId() const = 0;
    virtual CodeId getPaymentChannelCodeId() const = 0;
    virtual CodeId getRewardActorCodeId() const = 0;
    virtual CodeId getSystemActorCodeId() const = 0;
    virtual CodeId getVerifiedRegistryCodeId() const = 0;
  };

  using AddressMatcherPtr = std::shared_ptr<AddressMatcher>;

}  // namespace fc::vm::toolchain
