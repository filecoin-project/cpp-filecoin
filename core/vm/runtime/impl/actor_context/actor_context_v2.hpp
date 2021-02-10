/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/runtime/actor_context.hpp"

namespace fc::vm::runtime::context {
  class ActorContextV2 : public ActorContext {
   public:
    ActorContextV2() = default;

    bool isAccountActor(const CodeId &code) const override;
    bool isStorageMinerActor(const CodeId &code) const override;
    bool isBuiltinActor(const CodeId &code) const override;
    bool isSingletonActor(const CodeId &code) const override;
    bool isSignableActor(const CodeId &code) const override;

    CodeId getAccountCodeCid() const override;
    CodeId getCronCodeCid() const override;
    CodeId getStoragePowerCodeCid() const override;
    CodeId getStorageMarketCodeCid() const override;
    CodeId getStorageMinerCodeCid() const override;
    CodeId getMultisigCodeCid() const override;
    CodeId getInitCodeCid() const override;
    CodeId getPaymentChannelCodeCid() const override;
    CodeId getRewardActorCodeID() const override;
    CodeId getSystemActorCodeID() const override;
    CodeId getVerifiedRegistryCodeId() const override;
  };
}  // namespace fc::vm::runtime::context
