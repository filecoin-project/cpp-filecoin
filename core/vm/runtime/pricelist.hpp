/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::runtime {
  using primitives::GasAmount;
  using primitives::TokenAmount;
  using primitives::sector::AggregateSealVerifyProofAndInfos;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::WindowPoStVerifyInfo;

  struct Pricelist {
    // TODO (turuslan): lotus bug, lotus uses genesis prices for negative
    // upgrades
    explicit Pricelist(ChainEpoch epoch)
        : calico{kUpgradeCalicoHeight > 0 && epoch >= kUpgradeCalicoHeight} {}
    static inline GasAmount make(GasAmount compute, GasAmount storage) {
      return compute + storage;
    }
    inline GasAmount storage(size_t size) const {
      return (calico ? 1300 : 1000) * static_cast<GasAmount>(size);
    }
    inline GasAmount onChainMessage(size_t size) const {
      return make(38863, storage(36 + size));
    }
    inline GasAmount onChainReturnValue(size_t size) const {
      return make(0, storage(size));
    }
    static inline GasAmount onMethodInvocation(const TokenAmount &value,
                                               uint64_t method) {
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
    static inline GasAmount onVerifySignature(bool bls) {
      return make(bls ? 16598605 : 1637292, 0);
    }
    static inline GasAmount onHashing() {
      return make(31355, 0);
    }
    static inline GasAmount onComputeUnsealedSectorCid() {
      return make(98647, 0);
    }
    static inline GasAmount onVerifySeal() {
      return make(2000, 0);
    }
    inline GasAmount onVerifyPost(const WindowPoStVerifyInfo &info) const {
      GasAmount flat{123861062};
      GasAmount scale{9226981};
      if (calico) {
        flat = 117680921;
        scale = 43780;
      } else if (!info.proofs.empty()) {
        auto type{info.proofs[0].registered_proof};
        if (type == RegisteredPoStProof::kStackedDRG32GiBWindowPoSt
            || type == RegisteredPoStProof::kStackedDRG64GiBWindowPoSt) {
          flat = 748593537;
          scale = 85639;
        }
      }
      GasAmount gas =
          flat + scale * static_cast<GasAmount>(info.challenged_sectors.size());
      if (!calico) {
        gas /= 2;
      }
      return make(gas, 0);
    }
    GasAmount onVerifyAggregateSeals(
        const AggregateSealVerifyProofAndInfos &aggregate) const {
      if (!calico) {
        return 0;
      }
      using Pair = std::pair<size_t, GasAmount>;
      static const std::array<std::array<Pair, 9>, 2> kSteps{{
          {
              Pair{0, 0},
              Pair{4, 103994170},
              Pair{7, 112356810},
              Pair{13, 122912610},
              Pair{26, 137559930},
              Pair{52, 162039100},
              Pair{103, 210960780},
              Pair{205, 318351180},
              Pair{410, 528274980},
          },
          {
              Pair{0, 0},
              Pair{4, 102581240},
              Pair{7, 110803030},
              Pair{13, 120803700},
              Pair{26, 134642130},
              Pair{52, 157357890},
              Pair{103, 203017690},
              Pair{205, 304253590},
              Pair{410, 509880640},
          },
      }};
      const auto n{aggregate.infos.size()};
      const auto _64gib{aggregate.seal_proof
                        == RegisteredSealProof::kStackedDrg64GiBV1_1};
      const auto &steps{_64gib ? kSteps[1] : kSteps[0]};
      const auto step{std::prev(std::upper_bound(steps.begin(),
                                                 steps.end(),
                                                 Pair{n, {}},
                                                 [](auto &l, auto &r) {
                                                   return l.first < r.first;
                                                 }))
                          ->second};
      return static_cast<GasAmount>(n) * (_64gib ? 359272 : 449900) + step;
    }
    GasAmount onVerifyReplicaUpdate() {
      return make(calico ? 36316136 : 0, 0);
    }
    static inline GasAmount onVerifyConsensusFault() {
      return make(495422, 0);
    }

    bool calico{};
  };
}  // namespace fc::vm::runtime
