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
    inline GasAmount make(GasAmount compute, GasAmount storage) const {
      return compute + storage;
    }
    inline GasAmount storage(GasAmount gas) const {
      return (calico ? 1300 : 1000) * gas;
    }
    inline GasAmount onChainMessage(size_t size) const {
      return make(38863, storage(36 + size));
    }
    inline GasAmount onChainReturnValue(size_t size) const {
      return make(0, storage(size));
    }
    inline GasAmount onMethodInvocation(TokenAmount value,
                                        uint64_t method) const {
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
    inline GasAmount onIpldGet() const {
      return make(calico ? 114617 : 75242, 0);
    }
    inline GasAmount onIpldPut(size_t size) const {
      return make(calico ? 353640 : 84070, storage(size));
    }
    inline GasAmount onCreateActor() const {
      return make(1108454, storage(36 + 40));
    }
    inline GasAmount onDeleteActor() const {
      return make(0, storage(-(36 + 40)));
    }
    inline GasAmount onVerifySignature(bool bls) const {
      return make(bls ? 16598605 : 1637292, 0);
    }
    inline GasAmount onHashing() const {
      return make(31355, 0);
    }
    inline GasAmount onComputeUnsealedSectorCid() const {
      return make(98647, 0);
    }
    inline GasAmount onVerifySeal() const {
      return make(2000, 0);
    }
    inline GasAmount onVerifyPost(const WindowPoStVerifyInfo &info) const {
      int64_t flat{123861062}, scale{9226981};
      if (calico) {
        flat = 117680921;
        scale = 43780;
      } else if (!info.proofs.empty()) {
        auto type{info.proofs[0].registered_proof};
        if (type == RegisteredProof::StackedDRG32GiBWindowPoSt
            || type == RegisteredProof::StackedDRG64GiBWindowPoSt) {
          flat = 748593537;
          scale = 85639;
        }
      }
      auto gas{flat + scale * info.challenged_sectors.size()};
      if (!calico) {
        gas /= 2;
      }
      return make(gas, 0);
    }
    inline GasAmount onVerifyConsensusFault() const {
      return make(495422, 0);
    }

    bool calico{};
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_PRICELIST_HPP
