/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types {
  using miner::ExpirationQueuePtr;
  using miner::ExpirationSet;
  using miner::QuantSpec;
  using runtime::Runtime;

  class TypeManager {
   public:
    static outcome::result<ExpirationQueuePtr> loadExpirationQueue(
        Runtime &runtime,
        const adt::Array<ExpirationSet> &expirations_epochs,
        const QuantSpec &quant);
  };
}  // namespace fc::vm::actor::builtin::types
