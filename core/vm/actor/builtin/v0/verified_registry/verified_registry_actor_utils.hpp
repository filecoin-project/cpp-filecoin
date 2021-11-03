/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/verified_registry_actor_utils.hpp"

namespace fc::vm::actor::builtin::v0::verified_registry {
  using primitives::StoragePower;
  using runtime::Runtime;

  class VerifRegUtils : public utils::VerifRegUtils {
   public:
    explicit VerifRegUtils(Runtime &r) : utils::VerifRegUtils(r) {}

    outcome::result<void> checkDealSize(
        const StoragePower &deal_size) const override;
  };
}  // namespace fc::vm::actor::builtin::v0::verified_registry
