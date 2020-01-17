/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INDICES_HPP
#define CPP_FILECOIN_CORE_VM_INDICES_HPP

#include <primitives/big_int.hpp>

namespace fc::vm {
  class Indices {
   public:
    static fc::primitives::BigInt ConsensusPowerForStorageWeight(
        int storage_weight_desc);
  };
}  // namespace fc::vm

#endif  // CPP_FILECOIN_CORE_VM_INDICES_HPP
