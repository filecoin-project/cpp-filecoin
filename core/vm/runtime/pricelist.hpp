/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP
#define FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP

#include "primitives/types.hpp"

namespace fc::vm::runtime {
  using primitives::GasAmount;
  using primitives::TokenAmount;

  struct Pricelist {
    GasAmount onChainMessage(size_t size) const {
      return 0 + size * 2;
    }
    GasAmount onChainReturnValue(size_t size) const {
      return size * 8;
    }
    GasAmount onMethodInvocation(TokenAmount value, uint64_t method) const {
      return 5 + (value != 0 ? 5 : 0) + (method != 0 ? 10 : 0);
    }
    GasAmount onIpldGet(size_t size) const {
      return 10 + size * 1;
    }
    GasAmount onIpldPut(size_t size) const {
      return 20 + size * 2;
    }
    GasAmount onVerifySignature(size_t size) const {
      return 2 + size * 3;
    }
    GasAmount onComputeUnsealedSectorCid() const {
      return 100;
    }
    GasAmount onVerifySeal() const {
      return 2000;
    }
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP
