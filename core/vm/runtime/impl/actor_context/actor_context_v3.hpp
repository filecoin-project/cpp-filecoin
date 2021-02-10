/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/runtime/actor_context.hpp"

namespace fc::vm::runtime::context {
  class ActorContextV3 : public ActorContext {
   public:
    ActorContextV3() = default;

    bool isAccountActor(const CodeId &code) const override;
    bool isStorageMinerActor(const CodeId &code) const override;
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
}  // namespace fc::vm::runtime::context
