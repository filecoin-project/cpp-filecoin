/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address_codec.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::address::Address;

  struct WorkerKeyChange {
    /// Must be an ID address
    Address new_worker;
    ChainEpoch effective_at{};

    inline bool operator==(const WorkerKeyChange &other) const {
      return new_worker == other.new_worker
             && effective_at == other.effective_at;
    }
  };
  CBOR_TUPLE(WorkerKeyChange, new_worker, effective_at)

}  // namespace fc::vm::actor::builtin::types::miner
