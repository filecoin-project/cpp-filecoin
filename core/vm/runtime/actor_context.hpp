/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#pragma once

namespace fc::vm::runtime::context {
  using actor::CodeId;

  class ActorContext {
   public:
    virtual ~ActorContext() = default;

    /** Checks if code is an account actor */
    virtual bool isAccountActor(const CodeId &code) const = 0;

    /** Checks if code is miner actor */
    virtual bool isStorageMinerActor(const CodeId &code) const = 0;

    /** Check if code specifies builtin actor implementation */
    virtual bool isBuiltinActor(const CodeId &code) const = 0;

    /** Check if only one instance of actor should exists */
    virtual bool isSingletonActor(const CodeId &code) const = 0;

    /** Check if actor code can represent external signing parties */
    virtual bool isSignableActor(const CodeId &code) const = 0;

    /** Get actor's code id */
    virtual CodeId getAccountCodeCid() const = 0;
    virtual CodeId getCronCodeCid() const = 0;
    virtual CodeId getStoragePowerCodeCid() const = 0;
    virtual CodeId getStorageMarketCodeCid() const = 0;
    virtual CodeId getStorageMinerCodeCid() const = 0;
    virtual CodeId getMultisigCodeCid() const = 0;
    virtual CodeId getInitCodeCid() const = 0;
    virtual CodeId getPaymentChannelCodeCid() const = 0;
    virtual CodeId getRewardActorCodeID() const = 0;
    virtual CodeId getSystemActorCodeID() const = 0;
    virtual CodeId getVerifiedRegistryCodeId() const = 0;
  };

  using ActorContextPtr = std::shared_ptr<ActorContext>;

}  // namespace fc::vm::runtime::context
