/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::market {
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::SectorSize;

  constexpr MethodNumber kVerifyDealsOnSectorProveCommitMethodNumber{6};
  constexpr MethodNumber kOnMinerSectorsTerminateMethodNumber{7};
  constexpr MethodNumber kComputeDataCommitmentMethodNumber{8};

  struct VerifyDealsOnSectorProveCommitParams {
    std::vector<DealId> deals;
    ChainEpoch sector_expiry;
  };

  struct OnMinerSectorsTerminateParams {
    std::vector<DealId> deals;
  };

  struct ComputeDataCommitmentParams {
    std::vector<DealId> deals;
    SectorSize sector_size;
  };

  CBOR_TUPLE(VerifyDealsOnSectorProveCommitParams, deals, sector_expiry)

  CBOR_TUPLE(OnMinerSectorsTerminateParams, deals)

  CBOR_TUPLE(ComputeDataCommitmentParams, deals, sector_size)
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
