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
  using primitives::DealWeight;
  using primitives::SectorSize;

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

  struct VerifyDealsOnSectorProveCommit : ActorMethodBase<6> {
    using Params = VerifyDealsOnSectorProveCommitParams;
    using Result = DealWeight;
    ACTOR_METHOD_DECL();
  };

  struct OnMinerSectorsTerminate : ActorMethodBase<7> {
    using Params = OnMinerSectorsTerminateParams;
    ACTOR_METHOD_DECL();
  };

  struct ComputeDataCommitment : ActorMethodBase<8> {
    using Params = ComputeDataCommitmentParams;
    using Result = CID;
    ACTOR_METHOD_DECL();
  };

  CBOR_TUPLE(VerifyDealsOnSectorProveCommitParams, deals, sector_expiry)

  CBOR_TUPLE(OnMinerSectorsTerminateParams, deals)

  CBOR_TUPLE(ComputeDataCommitmentParams, deals, sector_size)
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
