/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/ipfs/datastore.hpp"
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

   private:
    template <typename T>
    static std::shared_ptr<T> createLoadedExpirationQueuePtr(
        IpldPtr ipld,
        const adt::Array<ExpirationSet> &expirations_epochs,
        const QuantSpec &quant) {
      auto eq = std::make_shared<T>();
      eq->queue = expirations_epochs;
      eq->quant = quant;
      cbor_blake::cbLoadT(ipld, *eq);
      return eq;
    }
  };
}  // namespace fc::vm::actor::builtin::types
