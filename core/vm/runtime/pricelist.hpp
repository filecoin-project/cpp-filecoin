/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP
#define FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::runtime {
  using primitives::GasAmount;
  using primitives::TokenAmount;
  using primitives::sector::WindowPoStVerifyInfo;

  struct Pricelist {
    GasAmount make(GasAmount compute, GasAmount storage) const {
      return compute + storage * 1000;
    }
    GasAmount onChainMessage(size_t size) const {
      return make(38863, 36 + size);
    }
    GasAmount onChainReturnValue(size_t size) const {
      return make(0, size);
    }
    GasAmount onMethodInvocation(TokenAmount value, uint64_t method) const {
      GasAmount gas{29233};
      if (value != 0) {
        gas += 27500;
        if (method == 0) {
          gas += 159672;
        }
      }
      if (method != 0) {
        gas += -5377;
      }
      return make(gas, 0);
    }
    GasAmount onIpldGet() const {
      return make(75242, 0);
    }
    GasAmount onIpldPut(size_t size) const {
      return make(84070, size);
    }
    GasAmount onCreateActor() const {
      return make(1108454, 36 + 40);
    }
    GasAmount onDeleteActor() const {
      return make(0, -(36 + 40));
    }
    GasAmount onVerifySignature(bool bls) const {
      return make(bls ? 16598605 : 1637292, 0);
    }
    GasAmount onHashing() const {
      return make(31355, 0);
    }
    GasAmount onComputeUnsealedSectorCid() const {
      return make(98647, 0);
    }
    GasAmount onVerifySeal() const {
      return make(2000, 0);
    }
    GasAmount onVerifyPost(const WindowPoStVerifyInfo &info) const {
      int64_t flat{123861062}, scale{9226981};
      if (!info.proofs.empty()) {
        auto type{info.proofs[0].registered_proof};
        if (type == RegisteredProof::StackedDRG32GiBWindowPoSt
            || type == RegisteredProof::StackedDRG64GiBWindowPoSt) {
          flat = 748593537;
          scale = 85639;
        }
      }
      return make((flat + scale * info.challenged_sectors.size()) / 2, 0);
    }
    GasAmount onVerifyConsensusFault() const {
      return make(495422, 0);
    }
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP
